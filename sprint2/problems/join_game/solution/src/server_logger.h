//
// Created by Pavel on 19.08.2024.
//
#pragma once

#include <boost/json.hpp>
#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/date_time.hpp>       // для вывода даты
#include <boost/log/utility/setup/file.hpp>  //для вывода лога в файл
#include <boost/log/utility/setup/common_attributes.hpp> //для вывода плейсхолдеров %LineID% %TimeStamp% etc
#include <boost/log/utility/setup/console.hpp> //для вывода в консоль
#include <boost/log/utility/manipulators/add_value.hpp>

#include <boost/beast.hpp>

#include <string_view>

//Что бы упростить конструкцию с logging::extract
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

BOOST_LOG_ATTRIBUTE_KEYWORD(log_message, "message", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(log_msg_data, "data", boost::json::value)

namespace server_logger {
    namespace net = boost::asio;
    namespace http = boost::beast::http;
    namespace json = boost::json;
    namespace logging = boost::log;

    using namespace std::literals;

    void InitLogging();
    void JsonFormatter(logging::record_view const &rec, logging::formatting_ostream &strm);

    using tcp = net::ip::tcp;
    using namespace std::chrono;

    //Обертка над классом RequestHandler выполняющая логгирование запросов и ответов
    template <class RequestHandler>
    class LoggingRequestHandler
    {
        template <typename Request>
        static void LogRequest(const tcp::endpoint& endpoint, const Request& req)
        {
            json::object additional_data{
                    {"ip", endpoint.address().to_string()},
                    {"URI", req.target()},
                    {"method", req.method_string()}
            };

            BOOST_LOG_TRIVIAL(info) << logging::add_value(log_message, "request received"s)
                                    << logging::add_value(log_msg_data, additional_data);
        }

        template <typename Response>
        static void LogResponse(auto start_ts, const Response& res)
        {
            high_resolution_clock::time_point end_ts = high_resolution_clock::now();
            auto msec = duration_cast<milliseconds>(end_ts - start_ts).count();

            json::object additional_data{
                    {"response_time", msec},
                    {"code", res.result_int()},
            };

            auto content_type = res[http::field::content_type];
            if (content_type.empty())
            {
                additional_data["content_type"] = nullptr;
            }
            else
            {
                additional_data["content_type"] = std::string(content_type);
            }

            BOOST_LOG_TRIVIAL(info) << logging::add_value(log_message, "response sent"s)
                                    << logging::add_value(log_msg_data, additional_data);
        }

    public:
        LoggingRequestHandler(RequestHandler handler)
        : handler_(std::forward<RequestHandler>(handler)) {
        }

        void operator ()(tcp::endpoint&& endpoint, auto&& req, auto&& send)
        {

            LogRequest(endpoint, req);

            //Start timer for response
            high_resolution_clock::time_point start_ts = high_resolution_clock::now();

            auto log_and_send_response = [&](auto&& resp)
            {
                LogResponse(start_ts, resp);
                send(std::move(resp));
            };

            //End timer when logging response
            handler_(std::move(endpoint), std::forward<decltype(req)>(req), log_and_send_response);
        }

    private:
        RequestHandler handler_;
    };
}
