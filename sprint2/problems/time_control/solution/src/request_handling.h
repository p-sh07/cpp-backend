#pragma once
#include <optional>
#include <chrono>
#include <utility>
#include <variant>
#include <string>
#include <string_view>

#include "http_server.h"

#include "model.h"
#include "application.h"
#include "json_loader.h"

namespace http_handler {

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

// Ответ, тело которого представлено в виде содержимого файла
using FileResponse = http::response<http::file_body>;
// Пустой ответ
using EmptyResponse = http::response<http::empty_body>;

// Структура ContentType задаёт область видимости для констант,
// задающий значения HTTP-заголовка Content-Type
struct ContentType {
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


//===================================================================
//======================= Error types =======================

enum class ErrCode {
    bad_method,
    bad_request,
    bad_method_post_only,
    bad_method_get_head_only,

    //join_game:
    map_not_found,
    invalid_player_name,
    join_game_parse_err,

    //player_list:
    invalid_token,
    unknown_token,

    //game actions
    token_invalid_argument,
    invalid_content_type,
    time_tick_invalid_argument,
};

struct ErrInfo {
    http::status status;
    std::string_view code = "";
    std::string_view msg = "";
};

class ServerError : public std::runtime_error {
 public:
    ServerError(ErrCode ec)
        : ec_(ec)
        , runtime_error("Server Error") {
    }

    virtual ErrCode ec() const { return ec_; }
    virtual http::status status() const { return GetInfo(ec_).status; }
    virtual std::string_view message() const { return GetInfo(ec_).msg; }

 protected:
    ErrCode ec_;

    virtual ErrInfo GetInfo(ErrCode ec) const {
        switch(ec) {
            case ErrCode::bad_method:
                return {http::status::method_not_allowed, ""sv, "Invalid method"sv};
        }
    }

};

class ApiError : public ServerError {
 public:
    using ServerError::ServerError;
    //TODO: add optional message to override one in error

    std::string print_json() const {
        auto err_info = GetInfo(ec_);
        json::value jv{
            {"code", err_info.code},
            {"message", err_info.msg}
        };
        return json::serialize(jv);
    }

 private:

    ErrInfo GetInfo(ErrCode ec) const override {
        switch(ec) {
            case ErrCode::bad_request:
                return {http::status::bad_request,
                        "badRequest"sv,
                        "Bad request"sv};
                break;
            case ErrCode::bad_method_get_head_only:
                return {http::status::method_not_allowed,
                        "invalidMethod"sv,
                        "Invalid method in Player list request"sv};
                break;
            case ErrCode::map_not_found:
                return {http::status::not_found,
                        "mapNotFound"sv,
                        "Map not found"sv};
                break;
            case ErrCode::invalid_player_name:
                return {http::status::bad_request,
                        "invalidArgument"sv,
                        "Invalid name"sv};
                break;
            case ErrCode::join_game_parse_err:
                return {http::status::bad_request,
                        "invalidArgument"sv,
                        "Join game request parse error"sv};
                break;
            case ErrCode::bad_method_post_only:
                return {http::status::method_not_allowed,
                        "invalidMethod"sv,
                        "Only POST method is expected"sv};
                break;
            case ErrCode::invalid_token:
                return {http::status::unauthorized,
                        "invalidToken"sv,
                        "Auth header not found or incorrect token format"sv};
                break;
            case ErrCode::unknown_token:
                return {http::status::unauthorized,
                        "unknownToken"sv,
                        "Player token has not been found"sv};
                break;
            case ErrCode::token_invalid_argument:
                return {http::status::unauthorized,
                        "invalidArgument"sv,
                        "Failed to parse json"sv};
                break;
            case ErrCode::invalid_content_type:
                return {http::status::unauthorized,
                        "invalidArgument"sv,
                        "Invalid content type"sv};
                break;
            case ErrCode::time_tick_invalid_argument:
                return {http::status::bad_request,
                        "badRequest"sv,
                        "Failed to parse tick request"sv};
                break;

        }
    }
};

class FileError : public ServerError {
 public:
    using ServerError::ServerError;
};


//===================================================================
//======================= Common =======================
namespace {
// Определить MIME-тип из расширения запрашиваемого файла
std::string_view ParseMimeType(const std::string_view& path);

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body,
                                  unsigned http_version, bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML);

http::response<http::file_body> MakeResponseFromFile(const char* file_path, unsigned http_version, bool keep_alive);

//TODO: move to utils?
//Возвращает true, если каталог p содержится внутри base.
bool IsSubPath(fs::path path, fs::path base);

//Конвертирует URL-кодированную строку в путь
fs::path ConvertFromUrl(std::string_view url);
} //local namespace
//===================================================================
//======================= Api Request Handler =======================

class ApiHandler : public std::enable_shared_from_this<ApiHandler> {
 public:
    using Strand = net::strand<net::io_context::executor_type>;
    ApiHandler(Strand api_strand, std::shared_ptr<app::GameInterface> game_app);

    ApiHandler(const ApiHandler&) = delete;
    ApiHandler& operator=(const ApiHandler&) = delete;

    template<typename Request>
    bool IsApiRequest(const Request& req) const { return req.target().starts_with(Uri::api); }

    template<typename Body, typename Allocator, typename Send>
    void Execute(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

 private:
    struct Uri {
        //var
        static constexpr std::string_view bearer{"Bearer "sv};
        static constexpr std::string_view api{"/api/"sv};

        //api calls, after /api/
        static constexpr std::string_view map_list{"v1/maps"sv};
        static constexpr std::string_view game{"v1/game/"sv};

        //game calls, after /api/v1/game/
        static constexpr std::string_view game_state{"state"sv};
        static constexpr std::string_view join_game{"join"sv};
        static constexpr std::string_view player_list{"players"sv};
        static constexpr std::string_view player_action{"player/action"sv};
        static constexpr std::string_view time_tick{"tick"sv};
    };

    Strand strand_;
    std::shared_ptr<app::GameInterface> game_app_;

    std::string_view ExtractMapId(std::string_view uri) const;
    static std::pair<std::string, std::string> ExtractMapIdPlayerName (const std::string& request_body);

    std::string ExtractToken(const auto& request) const;
    app::PlayerPtr AuthorizePlayer(const auto& request) const;

    static bool RemoveIfHasPrefix(std::string_view prefix, std::string_view& uri);

    StringResponse HandleApiRequest(const StringRequest& req);

    StringResponse ReportApiError(const ApiError& err, unsigned version, bool keep_alive) const;
    StringResponse ReportApiError(unsigned version, bool keep_alive) const;
};

template<typename Body, typename Allocator, typename Send>
void ApiHandler::Execute(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
    auto version = req.version();
    auto keep_alive = req.keep_alive();

    //currently all api requests performed inside one strand consecutively
    //TODO: switch to one strand per Session
    try {
        auto handle = [self = shared_from_this(), send,
            req = std::move(req), version, keep_alive] {
            try {
                return send(self->HandleApiRequest(req));
            } catch(const ApiError& err) {
                send(self->ReportApiError(err, version, keep_alive));
            }
            catch(...) {
                send(self->ReportApiError(version, keep_alive));
            }
        };

        return net::dispatch(strand_, handle);
    }
    catch(...) {
        //TODO: What error can be caught here?
        send(ReportApiError(version, keep_alive));
    }
}


//===================================================================
//======================= File Request Handler ======================
class FileHandler : public std::enable_shared_from_this<FileHandler> {
 public:
    using Strand = net::strand<net::io_context::executor_type>;
    FileHandler(fs::path root);

    FileHandler(const FileHandler&) = delete;
    FileHandler& operator=(const FileHandler&) = delete;

    template<typename Body, typename Allocator, typename Send>
    void Execute(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

 private:
    using FileRequestResult = std::variant<EmptyResponse, StringResponse, FileResponse>;
    fs::path root_;

    FileRequestResult HandleFileRequest(const StringRequest& req) const;
    StringResponse ReportFileError(const FileError& err, unsigned version, bool keep_alive) const;
    StringResponse ReportFileError(unsigned version, bool keep_alive) const;
};

template<typename Body, typename Allocator, typename Send>
void FileHandler::Execute(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
    auto version = req.version();
    auto keep_alive = req.keep_alive();

    try {
        // Возвращаем результат обработки запроса к файлу
        return std::visit(
            [&send](auto&& result) {
                send(std::forward<decltype(result)>(result));
            },
            HandleFileRequest(req));
    } catch(const FileError& err) {
        send(ReportFileError(err, version, keep_alive));
    }catch(...) {
        send(ReportFileError(version, keep_alive));
    }
}


//===================================================================
//======================= Request Handling Interface ================
class RequestHandler {
 public:
    using Strand = net::strand<net::io_context::executor_type>;

    RequestHandler(fs::path root, Strand api_strand, std::shared_ptr<app::GameInterface> game_app)
        : file_handler_(std::make_shared<FileHandler>(std::move(root)))
        , api_handler_(std::make_shared<ApiHandler>(api_strand, std::move(game_app))) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template<typename Body, typename Allocator, typename Send>
    void operator()(tcp::endpoint&&, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

 private:

    std::shared_ptr<ApiHandler> api_handler_;
    std::shared_ptr<FileHandler> file_handler_;

    template<typename Request>
    void AssertRequestValid(const Request& req);

    StringResponse ReportServerError(const ServerError& err, unsigned version, bool keep_alive) const;
    StringResponse ReportServerError(unsigned version, bool keep_alive) const;
};

template<typename Request>
void RequestHandler::AssertRequestValid(const Request& req) {
    // Unknown method
    if(req.method() != http::verb::get && req.method() != http::verb::head && req.method() != http::verb::post) {
        throw ServerError(ErrCode::bad_method);
    }

    //NB: Can add other cases:
}

template<typename Body, typename Allocator, typename Send>
void RequestHandler::operator()(tcp::endpoint&&, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
    auto version = req.version();
    auto keep_alive = req.keep_alive();

    try {
        //Throws ServerError if the request cannot be processed
        //AssertRequestValid(req);

        if(api_handler_->IsApiRequest(req)) {
            return api_handler_->Execute(std::move(req), send);
        }
        // Возвращаем результат обработки запроса к файлу
        file_handler_->Execute(std::move(req), send);
    } catch(ServerError& err) {
        send(ReportServerError(err, version, keep_alive));
    } catch(...) {
        send(ReportServerError(version, keep_alive));
    }
}

} // namespace http_handler
