#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <ranges>
#include <string>

namespace beast = boost::beast;
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace http = beast::http;
namespace websocket = beast::websocket;

using tcp = boost::asio::ip::tcp;
using namespace std::chrono_literals;

auto connect(websocket::stream<ssl::stream<beast::tcp_stream>>& stream,
			 std::string_view host,
			 std::string_view port) -> net::awaitable<void>
{
	auto executor = co_await net::this_coro::executor;
	auto resolver = net::ip::tcp::resolver{executor};
	const auto endpoints = co_await resolver.async_resolve(host, port);

	beast::get_lowest_layer(stream).expires_after(30s);
	if(!SSL_set_tlsext_host_name(stream.next_layer().native_handle(), host.data()))
	{
		throw beast::system_error(
			beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
			"Failed to set SNI Hostname");
	}

	auto ep = co_await beast::get_lowest_layer(stream).async_connect(endpoints);

	beast::get_lowest_layer(stream).expires_never();
}

auto handshake(websocket::stream<ssl::stream<beast::tcp_stream>>& stream,
			   std::string_view host,
			   std::string_view port,
			   std::string_view target) -> net::awaitable<void>
{
	beast::get_lowest_layer(stream).expires_after(30s);
	co_await stream.next_layer().async_handshake(ssl::stream_base::client);

	stream.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
	stream.set_option(websocket::stream_base::decorator([](websocket::request_type& req) {
		static const std::string userAgent =
			std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-coro";
		req.set(http::field::user_agent, userAgent);
	}));

	auto httpHost = std::format("{}:{}", host, port);

	// Perform the websocket handshake
	co_await stream.async_handshake(httpHost, target);
	beast::get_lowest_layer(stream).expires_never();
}

auto doSession(std::string host,
			   std::string port,
			   std::string target,
			   std::string request) -> net::awaitable<void>
{
	auto executor = co_await net::this_coro::executor;
	auto sslContext = ssl::context{ssl::context::tlsv13_client};
	auto stream = websocket::stream<ssl::stream<beast::tcp_stream>>{executor, sslContext};
	co_await connect(stream, host, port);

	co_await handshake(stream, host, port, target);

	co_await stream.async_write(net::buffer(request), boost::asio::use_awaitable);

	for(auto i : std::views::iota(0, 50))
	{
		beast::flat_buffer buffer;
		co_await stream.async_read(buffer, boost::asio::use_awaitable);
		std::cout << beast::make_printable(buffer.data()) << std::endl;
	}

	co_await stream.async_close(websocket::close_code::normal, boost::asio::use_awaitable);
}

auto main(int argc, char** argv) -> int
{
	try
	{
		const std::string host = "ws.kraken.com";
		const std::string port = "443";
		const std::string target = "/v2";
		const std::string subscribeRequest = R"(
			{ 
				"method": "subscribe",
				 "params": {
					"channel": "ticker",
					"symbol": ["BTC/USD"] 
				}
			}
			)";

		// The io_context is required for all I/O
		net::io_context ioc;

		// Launch the asynchronous operation
		net::co_spawn(
			ioc, doSession(host, port, target, subscribeRequest), [](std::exception_ptr e) {
				if(e) std::rethrow_exception(e);
			});

		// Run the I/O service. The call will return when
		// the socket is closed.
		ioc.run();
	}
	catch(std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
}