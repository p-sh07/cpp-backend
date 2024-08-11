#pragma once
#include "http_server.h"
#include "model.h"
#include <string>

#include "json_loader.h"

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;

    using namespace std::literals;

    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;

    static constexpr std::string_view API_PREFIX{"/api/"sv};
    static constexpr std::string_view MAP_LIST_PREFIX{"/api/v1/maps"sv};

    // Структура ContentType задаёт область видимости для констант,
    // задающий значения HTTP-заголовка Content-Type
    struct ContentType {
        ContentType() = delete;

        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view APP_JSON = "application/json"sv;
        // При необходимости внутрь ContentType можно добавить и другие типы контента
    };

    //Получить Id карты из запроса
    model::Map::Id GetMapId(std::string_view uri);

    // Создаёт StringResponse с заданными параметрами
    StringResponse
    MakeStringResponse(http::status status, std::string_view body,
                       unsigned http_version, bool keep_alive,
                       std::string_view content_type = ContentType::APP_JSON);

    class RequestHandler {
    public:
        explicit RequestHandler(model::Game &game) : game_{game} {
        }

        RequestHandler(const RequestHandler &) = delete;

        RequestHandler &operator=(const RequestHandler &) = delete;

        template<typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator> > &&req,
                        Send &&send);

    private:
        model::Game &game_;
    };

    template<typename Body, typename Allocator, typename Send>
    void RequestHandler::operator()(http::request<Body, http::basic_fields<Allocator> > &&req, Send &&send) {
        auto to_html = [&](http::status status, std::string_view text) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive());
        };

        // Unknown method
        if (req.method() != http::verb::get) {
            auto response = to_html(http::status::method_not_allowed, "Invalid method"sv);
            response.set(http::field::allow, "GET"sv);

            send(response);
            return;
        }

        std::string_view request_uri = req.target();

        //Contains only '/api/'
        if (request_uri == API_PREFIX) {
            send(to_html(http::status::bad_request,
                         json_loader::PrintErrorMsgJson("badRequest", "Bad request"))
            );
            return;
        }

        //Request for map list
        if (request_uri == MAP_LIST_PREFIX) {
            send(to_html(http::status::ok,
                         json_loader::PrintMapList(game_))
            );
            return;
        }

        //Request a map - contains full prefix for map list
        if (request_uri.rfind(MAP_LIST_PREFIX, 0) == 0
            && MAP_LIST_PREFIX.size() + 1 < request_uri.size()) {
            // /api/v1/maps/{id-карты}
            const auto map = game_.FindMap(GetMapId(request_uri));

            if (map) {
                send(to_html(http::status::ok, json_loader::PrintMap(*map)));
            } else {
                send(to_html(http::status::not_found,
                             json_loader::PrintErrorMsgJson("mapNotFound", "Map not found"))
                );
            }
            return;
        }

        //Unknown uri
        send(to_html(http::status::not_found,
                     json_loader::PrintErrorMsgJson("pageNotFount", "Unknown request"))
        );
    }
} // namespace http_handler
