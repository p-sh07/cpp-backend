#pragma once
#include "http_server.h"
#include "model.h"
#include <string>
#include <optional>

#include "json_loader.h"

namespace http_handler {
    namespace fs = std::filesystem;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace sys = boost::system;

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
        //Text
        constexpr static std::string_view TEXT_HTML  = "text/html"sv;
        constexpr static std::string_view TEXT_CSS   = "text/css"sv;
        constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
        constexpr static std::string_view TEXT_JS    = "text/javascript"sv;

        //App
        constexpr static std::string_view APP_JSON  = "application/json"sv;
        constexpr static std::string_view APP_XML   = "application/xml"sv;
        constexpr static std::string_view APP_OTHER = "application/octet-stream"sv;

        //Image
        constexpr static std::string_view IMG_PNG = "image/png"sv;
        constexpr static std::string_view IMG_JPG = "image/jpeg"sv;
        constexpr static std::string_view IMG_GIF = "image/gif"sv;
        constexpr static std::string_view IMG_BMP = "image/bmp"sv;
        constexpr static std::string_view IMG_ICO = "image/vnd.microsoft.icon"sv;
        constexpr static std::string_view IMG_TIF = "image/tiff"sv;
        constexpr static std::string_view IMG_SVG = "image/svg+xml"sv;

        //Audio
        constexpr static std::string_view AUDIO_MP3 = "audio/mpeg"sv;
    };

    // Определить MIME-тип из расширения запрашиваемого файла
    std::string_view ParseMimeType(const std::string_view& path);

    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeStringResponse(http::status status, std::string_view body,
                       unsigned http_version, bool keep_alive,
                       std::string_view content_type = ContentType::TEXT_HTML);

    http::response<http::file_body> MakeResponseFromFile(const char* file_path, unsigned http_version, bool keep_alive);

    //Получить Id карты из запроса
    std::optional<model::Map::Id> ExtractMapId(std::string_view uri);

    //Возвращает true, если каталог p содержится внутри base_path.
    bool IsSubPath(fs::path path, fs::path base);

    //Конвертирует URL-кодированную строку в путь
    fs::path ConvertFromUrl(std::string_view url);

    class RequestHandler {
    public:
        explicit RequestHandler(model::Game &game, fs::path path_to_root)
        : game_{game}
        , static_root_(std::move(path_to_root)) {
        }

        RequestHandler(const RequestHandler &) = delete;

        RequestHandler &operator=(const RequestHandler &) = delete;

        template<typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator> > &&req,
                        Send &&send);

    private:
        model::Game &game_;
        fs::path static_root_;
    };

    template<typename Body, typename Allocator, typename Send>
    void RequestHandler::operator()(http::request<Body, http::basic_fields<Allocator> > &&req, Send &&send) {
        //String response for api requests
        auto to_html = [&](http::status status, std::string_view text, std::string_view content_type) {
            return MakeStringResponse(status, text, req.version(),
                                      req.keep_alive(), content_type);
        };

        // Unknown method
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            auto response = to_html(http::status::method_not_allowed,
                                    "Invalid method"sv,
                                    ContentType::TEXT_HTML
                                    );
            response.set(http::field::allow, "GET, HEAD"sv);

            send(response);
            return;
        }

        std::string_view request_uri = req.target();

        if(request_uri.starts_with(API_PREFIX)) {
            //Request for map list
            if (request_uri == MAP_LIST_PREFIX) {
                send(to_html(http::status::ok,
                             json_loader::PrintMapList(game_),
                             ContentType::APP_JSON)
                );
                return;
            }

            //Request a map - contains full prefix for map list
            if (auto map_id_opt = ExtractMapId(request_uri)) {
                // /api/v1/maps/{id-карты}
                const auto map = game_.FindMap(map_id_opt.value());

                if (map) {
                    send(to_html(http::status::ok,
                                 json_loader::PrintMap(*map),
                                 ContentType::APP_JSON)
                    );
                } else {
                    send(to_html(http::status::not_found,
                                 json_loader::PrintErrorMsgJson("mapNotFound", "Map not found"),
                                 ContentType::APP_JSON)
                    );
                }
                return;
            }

            //Bad request
            send(to_html(http::status::bad_request,
                         json_loader::PrintErrorMsgJson("badRequest", "Bad request"),
                         ContentType::APP_JSON)
            );
            return;
        }

        //Try filesystem request
        fs::path requested_file;
        if(request_uri == "/"sv || request_uri == "/index.html"sv) {
            //special case, return index.html
            requested_file = static_root_ / fs::path("index.html"s);
        } else {
            //remove leading '/' to ensure correct splicing
            if(!request_uri.empty() && request_uri[0] == '/') {
                request_uri.remove_prefix(1);
            }
            requested_file = fs::weakly_canonical(static_root_ / request_uri);
        }

        //Error if requested file does not exist
        if(!fs::exists(requested_file)) {
            send(to_html(http::status::not_found,
                         "File not found"s,
                         ContentType::TEXT_PLAIN)
            );
            return;
        }
        //File outside filesystem root
        else if(!IsSubPath(requested_file, static_root_)) {
            send(to_html(http::status::bad_request,
                         "Access denied"s,
                         ContentType::TEXT_PLAIN)
            );
            return;
        }

        //File found, try to open & send
        send(MakeResponseFromFile(requested_file.c_str(), req.version(), req.keep_alive()));
    }
} // namespace http_handler
