#include <string>
#include <iostream>

//#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
//#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/context.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
namespace asio = boost::asio;
namespace ssl = asio::ssl;

using tcp = asio::ip::tcp;
using WSSsocket = websocket::stream<ssl::stream<tcp::socket>>;

using namespace std::string_literals;

auto ping(WSSsocket* socket) {
	std::cout << "Sending ping\n";
	constexpr std::string_view pingRequest = R"({"method":"ping"})";
		
	socket->write(asio::buffer(pingRequest));
	beast::flat_buffer response;
	
	std::cout << "Reading response\n";
	socket->read(response);
	std::cout << beast::make_printable(response.data()) << std::endl;
}

auto subscribe(WSSsocket* socket){
	
	constexpr std::string_view subscribeRequest = R"(
	{ 
		"method": "subscribe",
	 	"params": {
			"channel": "ticker",
			"symbol": ["BTC/USD"] 
		}
	}
	)";
	std::cout <<  "Sending subscribe\n";
	socket->write(asio::buffer(subscribeRequest));
	for (auto i = 0; i < 50; i++){
		beast::flat_buffer response;
		socket->read(response);
		std::cout << beast::make_printable(response.data()) <<"\n\n";
	}
}

void subscribeToQuotes(){
	constexpr std::string_view host = "ws.kraken.com";
    constexpr std::string_view port = "443";
    constexpr std::string_view target = "/v2"; 

	try {
		asio::io_context ioc;
		// The SSL context is used to set up SSL/TLS parameters.
		ssl::context ctx(ssl::context::tlsv13_client);
		tcp::resolver resolver(ioc);
        // Resolve the hostname to get IP addresses.
        const auto endpoints = resolver.resolve(host, port);
		for (const auto& endpoint : endpoints){
			std::cout << endpoint.endpoint().address().to_string() << "\n";
		}


        websocket::stream<ssl::stream<tcp::socket>> ws(ioc, ctx);
		const auto endpoint = asio::connect(ws.next_layer().next_layer(), endpoints.begin(), endpoints.end());
		std::cout << "Connected to " << endpoint->endpoint().address().to_string() << "\n";

		std::cout <<  "Performing SSL handshake\n";
		// Set SNI Hostname (many hosts need this to verify the SSL handshake)
        if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.data())) {
            throw beast::system_error(
                beast::error_code(static_cast<int>(::ERR_get_error()),
                                  asio::error::get_ssl_category()),
                "Failed to set SNI Hostname");
        }
		ws.next_layer().handshake(ssl::stream_base::client);
		std::cout <<  "SSL handhake complete\n";

		// Set a Host header for the WebSocket handshake.
        ws.set_option(websocket::stream_base::decorator(
            [host](websocket::request_type& req)
            {
                req.set(boost::beast::http::field::host, host);
            }));

		std::cout << "Perform websocket handshake\n";
		ws.handshake(host, target);
		std::cout << "Websocket handshake complete\n";

		//ping(&ws);
		subscribe(&ws);

		std::cout << "Closing socket\n";
		ws.close(websocket::close_code::normal);
	}
	 catch (const boost::system::system_error& e) {
        std::cerr << "Error: " <<  e.what();
    }
}

enum class HttpVersion:int{
	HTTP_11 = 11,
};

auto getIpAddresses(asio::io_context& context, std::string_view host, std::string_view port){
	// Резолвер адресов. По сути, разрешает доменные имена ("google.com") в набор реальных IP-адресов 
	// и соответствующих портов, к которым может подключиться сокет.
	// Если передан IP-адрес, просто проверяет его корректность и добавляет указанный порт.
	tcp::resolver resolver(context);
	return resolver.resolve(host, port);
}

auto makeHTTPRequest(http::verb type, std::string_view host, std::string_view target, HttpVersion httpVersion=HttpVersion::HTTP_11) -> http::request<http::string_body>
{
	// Обьект http реквеста. 
	http::request<http::string_body> request{http::verb::get, target, static_cast<int>(httpVersion)};
	// заполняем некоторые поля реквеста
    request.set(http::field::host, host);
    request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
	return request;
}

auto readResponse(beast::tcp_stream& stream)->http::response<http::dynamic_body>{
	// Буффер для хранения данных ответа. 
	beast::flat_buffer buffer;

	// А тут будет лежать уже готовый ответ
	http::response<http::dynamic_body> res;

	// Receive the HTTP response
	http::read(stream, buffer, res);
	return res;
}

void sendGetRequest(std::string_view host, std::string_view port, std::string_view target){
	namespace http = beast::http;
	
	// Обьект отвечающий за выполнение асинхронных операций.
	asio::io_context ioc;

	const auto addresses = getIpAddresses(ioc, host, port);

	// Обертка над TCP-сокетом для упрощения работы с ним
	beast::tcp_stream stream(ioc);
	
	// Подключаемся к первому доступнаму адресу из тех, что получили от резолвера.
	stream.connect(addresses);

	const auto request = makeHTTPRequest(http::verb::get, host, target);

	// Отправляем реквест
	http::write(stream, request);

	const auto response = readResponse(stream);
	std::cout << response << std::endl;

	// Аккуратно закрываем сокет
	beast::error_code ec; // Тут будет лежать код ошибки или код успеха
	stream.socket().shutdown(tcp::socket::shutdown_both, ec);

	// Проверяем код ошибки
	// В доках пишут, что иногда можно получить not_connected даже если все прошло ок, поэтому игнорируем эту ошбику
	if(ec && ec != beast::errc::not_connected)
		throw beast::system_error{ec};
}

auto main(int argc, char** argv) -> int
{
	constexpr auto host{"example.com"}; // тут можно протестить
	constexpr auto port{"80"}; //дефолтный порт для http
	constexpr auto target{"/"};

	try{
		sendGetRequest(host, port, target);
	}
	catch(const std::exception& e){
		std::cerr << "Next error occured: " << e.what();
	}

	return 0;
}