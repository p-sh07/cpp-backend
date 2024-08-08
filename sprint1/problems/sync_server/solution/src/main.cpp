#ifdef WIN32
#include <sdkddkver.h>
#endif
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = net::ip::tcp;

using namespace std::literals;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;


// Структура ContentType задаёт область видимости для констант,
// задающий значения HTTP-заголовка Content-Type
struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

StringResponse HandleRequest(StringRequest&& req) {
    auto text_response = [&](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive());
    };

    auto response_to_get = [&]() {
        //Make reply string "Hello, target" from '/target'
        std::string_view tg_content = req.target();
        tg_content.remove_prefix(1);
        std::string response_body = "Hello, " + std::string(tg_content);

        return text_response(http::status::ok, response_body);
    };

    auto request_type = req.method();

    //GET
    if(request_type == http::verb::get) {
        return response_to_get();
    }
    //HEAD
    else if(request_type == http::verb::head) {
        //make a GET response to set content-length field correctly
        auto response = response_to_get();
        response.body() = ""sv;

        return response;
    }

    //Other
    auto response = text_response(http::status::method_not_allowed, "Invalid method"sv);
    response.set(http::field::allow, "GET, HEAD"sv);

    return response;
}

std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer) {
    beast::error_code ec;
    StringRequest req;
    // Считываем из socket запрос req, используя buffer для хранения данных.
    // В ec функция запишет код ошибки.
    http::read(socket, buffer, req, ec);

    if (ec == http::error::end_of_stream) {
        return std::nullopt;
    }
    if (ec) {
        throw std::runtime_error("Failed to read request: "s.append(ec.message()));
    }
    return req;
}

void DumpRequest(const StringRequest& req) {
    std::cout << req.method_string() << ' ' << req.target() << std::endl;
    // Выводим заголовки запроса
    for (const auto& header : req) {
        std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
    }
}

template <typename RequestHandler>
void HandleConnection(tcp::socket& socket, RequestHandler&& handle_request) {
    try {

        // Буфер для чтения данных в рамках текущей сессии.
        beast::flat_buffer buffer;

        // Продолжаем обработку запросов, пока клиент их отправляет
        while (auto request = ReadRequest(socket, buffer)) {
            DumpRequest(*request);
            // Делегируем обработку запроса функции handle_request
            StringResponse response = handle_request(*std::move(request));
            http::write(socket, response);

            if (response.need_eof()) {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    beast::error_code ec;
    // Запрещаем дальнейшую отправку данных через сокет
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

int main() {
    net::io_context ioc;

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr unsigned short port = 8080;

    tcp::acceptor acceptor(ioc, {address, port});

    std::cout << "Server has started..."sv << std::endl;

    while (true) {
        tcp::socket socket(ioc);
        acceptor.accept(socket);

        // Запускаем обработку взаимодействия с клиентом в отдельном потоке
        std::thread t(
            // Лямбда-функция будет выполняться в отдельном потоке
            [](tcp::socket socket) {
                // Вызываем HandleConnection, передавая ей функцию-обработчик запроса
                HandleConnection(socket, HandleRequest);
            },
            std::move(socket));  // Сокет нельзя скопировать, но можно переместить

        // После вызова detach поток продолжит выполняться независимо от объекта t
        t.detach();
    }
}

//First example
/*
int main() {
    // Контекст для выполнения синхронных и асинхронных операций ввода/вывода
    net::io_context ioc;

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr unsigned short port = 8080;

    // Объект, позволяющий принимать tcp-подключения к сокету
    tcp::acceptor acceptor(ioc, {address, port});

    std::cout << "Waiting for socket connection"sv << std::endl;
    tcp::socket socket(ioc);
    acceptor.accept(socket);
    std::cout << "Connection received"sv << std::endl;

    constexpr std::string_view response
        = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello"sv;

    net::write(socket, net::buffer(response));
    std::cout << "Response sent, exiting:\n" << response << std::endl;
}
*/


//Class version ?
/*
class HttpServer {
public:
    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;

    HttpServer(std::string_view ip_addr_str = "0.0.0.0", unsigned short port = 8080)
        : ip_address_(net::ip::make_address_v4(ip_addr_str))
        , port_(port)
        , acceptor_(ioc_, {ip_address_, port_})
    {
        std::cout << "Server has started..."s << std::endl;
    }

    // Структура ContentType задаёт область видимости для констант,
    // задающий значения HTTP-заголовка Content-Type
    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        // При необходимости внутрь ContentType можно добавить и другие типы контента
    };

    void AcceptRequestAndRespond() {
            tcp::socket socket(ioc_);
            acceptor_.accept(socket);

            // Запускаем обработку взаимодействия с клиентом в отдельном потоке
            std::thread t(
                // Лямбда-функция будет выполняться в отдельном потоке
                [&](tcp::socket socket) {
                    HandleConnection(socket);
                },
                std::move(socket)); // Сокет нельзя скопировать, но можно переместить

            // После вызова detach поток продолжит выполняться независимо от объекта t
            t.detach();
    }

private:
    net::io_context ioc_;
    const net::ip::address_v4 ip_address_;
    const unsigned short port_;

    tcp::acceptor acceptor_;

    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeStringResponse(http::status status, std::string_view body,
                                      unsigned http_version, bool keep_alive,
                                      std::string_view content_type = ContentType::TEXT_HTML) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer) {
        beast::error_code ec;
        StringRequest req;
        // Считываем из socket запрос req, используя buffer для хранения данных.
        // В ec функция запишет код ошибки.
        http::read(socket, buffer, req, ec);

        if (ec == http::error::end_of_stream) {
            return std::nullopt;
        }
        if (ec) {
            throw std::runtime_error("Failed to read request: "s.append(ec.message()));
        }
        return req;
    }

    void DumpRequest(const StringRequest& req) {
        std::cout << req.method_string() << ' ' << req.target() << std::endl;
        // Выводим заголовки запроса
        for (const auto& header : req) {
            std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
        }
    }

    StringResponse HandleRequest(StringRequest&& req) {

        auto text_response = [&](http::status status, std::string_view text) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive());
        };

        auto response_to_get = [&]() {
            //Make reply string "Hello, target" from '/target'
            std::string_view tg_content = req.target();
            tg_content.remove_prefix(1);
            std::string response_body = "Hello, " + std::string(tg_content);

            return text_response(http::status::ok, response_body);
        };

        auto request_type = req.method();

        //GET
        if(request_type == http::verb::get) {
            return response_to_get();
        }
        //HEAD
        else if(request_type == http::verb::head) {
            //make a GET response to set content-length field correctly
            auto response = response_to_get();
            response.body() = ""sv;

            return response;
        }

        //Other
        auto response = text_response(http::status::method_not_allowed, "Invalid method"sv);
        response.set("Allow"s, "GET, HEAD"sv);

        return response;
    }

    void HandleConnection(tcp::socket& socket) {
        try {
            // Буфер для чтения данных в рамках текущей сессии.
            beast::flat_buffer buffer;
            while (auto request = ReadRequest(socket, buffer)) {
                DumpRequest(*request);
                // Делегируем обработку запроса функции HandleRequest
                StringResponse response = HandleRequest(*std::move(request));
                   http::write(socket, response);

                if (response.need_eof()) {
                   break;
                }
            }
        }  catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }

        beast::error_code ec;
        // Запрещаем дальнейшую отправку данных через сокет
        socket.shutdown(tcp::socket::shutdown_send, ec);
    }
};

int main() {

    //Use default params
    HttpServer my_server_;

    //Function will loop indefinitely and accept connections
    while (true) {
        my_server_.AcceptRequestAndRespond();
    }

    return 0;
}
*/
