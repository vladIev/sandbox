#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>

#include <format>
#include <iostream>
#include <memory>
#include <openssl/err.h>
#include <string_view>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
namespace ssl = asio::ssl;
using tcp = boost::asio::ip::tcp;
using namespace std::chrono_literals;

void fail(beast::error_code ec, std::string_view what)
{
	std::cerr << std::format("{}: {}\n", what, ec.message());
}

class Session : public std::enable_shared_from_this<Session>
{
	tcp::resolver d_resolver;
	websocket::stream<ssl::stream<beast::tcp_stream>> d_ws;
	beast::flat_buffer d_buffer;
	std::string d_host;
	std::string d_text;
	std::string d_target;

  public:
	explicit Session(asio::io_context& ioc, ssl::context& ctx)
		: d_resolver(asio::make_strand(ioc))
		, d_ws(asio::make_strand(ioc), ctx)
	{ }

	void run(std::string_view host,
			 std::string_view port,
			 std::string_view target,
			 std::string_view text)
	{
		d_host = host;
		d_text = text;
		d_target = target;

		d_resolver.async_resolve(
			host,
			port,
			[self = shared_from_this()](beast::error_code ec, tcp::resolver::results_type result) {
				self->onResolve(ec, result);
			});
	}

	void onResolve(beast::error_code ec, tcp::resolver::results_type result)
	{
		if(ec)
		{
			fail(ec, "resolve");
			return;
		}
		beast::get_lowest_layer(d_ws).expires_after(std::chrono::seconds(30));

		beast::get_lowest_layer(d_ws).async_connect(
			result,
			[self = shared_from_this()](beast::error_code ec,
										tcp::resolver::results_type::endpoint_type ep) {
				self->onConnect(ec, ep);
			});
	}

	void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
	{
		if(ec)
		{
			fail(ec, "connect");
			return;
		}

		beast::get_lowest_layer(d_ws).expires_after(std::chrono::seconds(30));

		// Set SNI Hostname (many hosts need this to verify the SSL handshake)
		if(!SSL_set_tlsext_host_name(d_ws.next_layer().native_handle(), d_host.data()))
		{
			fail(beast::error_code(static_cast<int>(::ERR_get_error()),
								   asio::error::get_ssl_category()),
				 "Set SNI Hostname");
			return;
			//throw beast::system_error(beast::error_code(static_cast<int>(::ERR_get_error()),
			//											asio::error::get_ssl_category()),
			//						  "Failed to set SNI Hostname");
		}

		d_host = std::format("{}:{}", d_host, ep.port());

		d_ws.next_layer().async_handshake(
			ssl::stream_base::client,
			[self = shared_from_this()](beast::error_code ec) { self->onSslHandshake(ec); });
	}

	void onSslHandshake(beast::error_code ec)
	{
		if(ec)
		{
			fail(ec, "sslhandshake");
			return;
		}

		beast::get_lowest_layer(d_ws).expires_after(std::chrono::seconds(30));

		d_ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
		d_ws.set_option(websocket::stream_base::decorator([](websocket::request_type& req) {
			static const std::string userAgent =
				std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async";
			req.set(http::field::user_agent, userAgent);
		}));

		// Perform the websocket handshake
		d_ws.async_handshake(d_host, d_target, [self = shared_from_this()](beast::error_code ec) {
			self->onHandshake(ec);
		});
	}

	void onHandshake(beast::error_code ec)
	{
		if(ec)
		{
			fail(ec, "handshake");
			return;
		}

		beast::get_lowest_layer(d_ws).expires_never();

		d_ws.async_write(
			asio::buffer(d_text),
			[self = shared_from_this()](beast::error_code ec, std::size_t bytesTransferred) {
				self->onWrite(ec, bytesTransferred);
			});
	}

	void onWrite(beast::error_code ec, [[maybe_unused]] std::size_t bytesTransferred)
	{
		if(ec)
		{
			fail(ec, "write");
			return;
		}

		d_ws.async_read(
			d_buffer,
			[self = shared_from_this()](beast::error_code ec, std::size_t bytesTransferred) {
				self->onRead(ec, bytesTransferred);
			});
	}

	void onRead(beast::error_code ec, [[maybe_unused]] std::size_t bytesTransferred)
	{
		if(ec)
		{
			fail(ec, "read");
			return;
		}
		std::cout << beast::make_printable(d_buffer.data()) << std::endl;
		d_buffer.clear();

		d_ws.async_read(
			d_buffer,
			[self = shared_from_this()](beast::error_code ec, std::size_t bytesTransferred) {
				self->onRead(ec, bytesTransferred);
			});

		//d_ws.async_close(websocket::close_code::normal,
		//				 [self = shared_from_this()](auto ec) { self->onClose(ec); });
	}

	void onClose(beast::error_code ec)
	{
		if(ec)
		{
			fail(ec, "close");
			return;
		}

		std::cout << beast::make_printable(d_buffer.data()) << std::endl;
	}
};

int main(int argc, char** argv)
{
	constexpr std::string_view host = "ws.kraken.com";
	constexpr std::string_view port = "443";
	constexpr std::string_view target = "/v2";

	asio::io_context ioc;
	auto sslContext = ssl::context{ssl::context::tlsv13_client};
	auto session = std::make_shared<Session>(ioc, sslContext);
	constexpr std::string_view subscribeRequest = R"(
        { 
            "method": "subscribe",
             "params": {
                "channel": "ticker",
                "symbol": ["BTC/USD"] 
            }
        }
        )";
	session->run(host, port, target, subscribeRequest);
	ioc.run();
	return 0;
}