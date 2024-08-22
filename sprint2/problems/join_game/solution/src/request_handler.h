#pragma once
#include <optional>
#include <chrono>
#include <utility>
#include <variant>
#include <string>
#include <string_view>

#include "model.h"
#include "player_manager.h"
#include "json_loader.h"
#include "http_server.h"

namespace http_handler
{
    namespace net = boost::asio;
    namespace sys = boost::system;
    namespace fs = std::filesystem;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace sys = boost::system;

    using tcp = net::ip::tcp;
    namespace json = boost::json;

    using namespace std::literals;

    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;

    //Ответ, тело которого представлено в виде содержимого файла
    using FileResponse = http::response<http::file_body>;
    // TODO: Пустой ответ
    using EmptyResponse = std::monostate;


    // Структура ContentType задаёт область видимости для констант,
    // задающий значения HTTP-заголовка Content-Type
    struct ContentType
    {
        ContentType() = delete;
        //Text
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view TEXT_CSS = "text/css"sv;
        constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
        constexpr static std::string_view TEXT_JS = "text/javascript"sv;

        //App
        constexpr static std::string_view APP_JSON = "application/json"sv;
        constexpr static std::string_view APP_XML = "application/xml"sv;
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

    class ApiError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class FileError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class ServerError : public std::runtime_error {
    public:
        ServerError(http::status status, std::string_view message)
        : status_(status)
        , runtime_error(message.data()){
        }

        inline http::status status() const {
            return status_;
        }
    private:
        http::status status_;
    };

    // Определить MIME-тип из расширения запрашиваемого файла
    std::string_view ParseMimeType(const std::string_view& path);

    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeStringResponse(http::status status, std::string_view body,
                                      unsigned http_version, bool keep_alive,
                                      std::string_view content_type = ContentType::TEXT_HTML);

    http::response<http::file_body> MakeResponseFromFile(const char* file_path, unsigned http_version, bool keep_alive);

    //Возвращает true, если каталог p содержится внутри base.
    bool IsSubPath(fs::path path, fs::path base);

    //Конвертирует URL-кодированную строку в путь
    fs::path ConvertFromUrl(std::string_view url);


    //======================= Api Request Handler ======================
    class ApiHandler : public std::enable_shared_from_this<ApiHandler> {
    public:
        using Strand = net::strand<net::io_context::executor_type>;
        ApiHandler(Strand& api_strand, std::shared_ptr<model::Game> game, std::shared_ptr<app::Players> players);

        ApiHandler(const ApiHandler &) = delete;
        ApiHandler &operator=(const ApiHandler &) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

    private:
        static constexpr std::string_view map_list_prefix_{"/api/v1/maps"sv};

        Strand& strand_;
        std::shared_ptr<model::Game> game_;
        std::shared_ptr<app::Players> players_;

        std::optional<model::Map::Id> ExtractMapId(std::string_view map_list_prefix, std::string_view uri);
        StringResponse HandleApiRequest(const StringRequest& req);
        StringResponse ReportApiError(unsigned version, bool keep_alive) const;
    };

    template <typename Body, typename Allocator, typename Send>
    void ApiHandler::operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();

        //currently all api requests performed inside one strand consecutively
        //TODO: switch to one strand per GameSession
        try {
            auto handle = [self = shared_from_this(), send,
                    req = std::move(req), version, keep_alive] {
                try {
                    return send(self->HandleApiRequest(req));
                } catch (...) {
                    send(self->ReportApiError(version, keep_alive));
                }
            };

            return net::dispatch(strand_, handle);
        }
        catch (...) {
            send(ReportApiError(version, keep_alive));
        }
    }


    //======================= File Request Handler ======================
    class FileHandler : public std::enable_shared_from_this<FileHandler> {
    public:
        using Strand = net::strand<net::io_context::executor_type>;
        FileHandler(fs::path root);

        FileHandler(const FileHandler &) = delete;
        FileHandler &operator=(const FileHandler &) = delete;

        template<typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send);

    private:
        using FileRequestResult = std::variant<EmptyResponse, StringResponse, FileResponse>;
        fs::path root_;

        FileRequestResult HandleFileRequest(const StringRequest& req) const;
        StringResponse ReportFileError(unsigned version, bool keep_alive) const;
    };

    template<typename Body, typename Allocator, typename Send>
    void FileHandler::operator()(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();

        try{
            // Возвращаем результат обработки запроса к файлу
            return std::visit(
                    [&send](auto &&result) {
                        send(std::forward<decltype(result)>(result));
                    },
                    HandleFileRequest(req));
        } catch (...) {
            //TODO: Error handling with throw?
            send(ReportServerError(version, keep_alive));
        }
    }


    //======================= Request Handling Interface ==========================
    class RequestHandler {
    public:
        using Strand = net::strand<net::io_context::executor_type>;

        RequestHandler(fs::path root, Strand api_strand, std::shared_ptr<model::Game> game, std::shared_ptr<app::Players> players)
                : file_handler_(std::make_shared<FileHandler>(std::move(root)))
                , api_handler_(std::make_shared<ApiHandler>(api_strand, std::move(game), std::move(players))) {
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(tcp::endpoint&&, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

    private:
        static constexpr std::string_view api_prefix_{"/api/"sv};

        std::shared_ptr<ApiHandler> api_handler_;
        std::shared_ptr<FileHandler> file_handler_;

        template<typename Request>
        void AssertRequestValid(const Request& req);

        StringResponse ReportServerError(const ServerError& err, unsigned version, bool keep_alive) const;
        StringResponse ReportServerError(unsigned version, bool keep_alive) const;
    };


    template<typename Request>
    void AssertRequestValid(const Request& req) {
        // Unknown method
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            throw ServerError(http::status::method_not_allowed, "Invalid method"sv);
        }

        //NB: Can add other cases:
    }

    template <typename Body, typename Allocator, typename Send>
    void RequestHandler::operator()(tcp::endpoint&&, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();

        try {
            AssertRequestValid(req);
            std::string_view uri = req.target();
            if (uri.starts_with(api_prefix_)) {
                return (*api_handler_)(std::move(req), send);
            }
            // Возвращаем результат обработки запроса к файлу
            (*file_handler_)(std::move(req), send);
        } catch (ServerError& err) {
            send(ReportServerError(err, version, keep_alive));
        } catch (...) {
            send(ReportServerError(version, keep_alive));
        }
    }


} // namespace http_handler
