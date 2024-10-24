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
    std_exception, //TODO: Add std exception with message from ex.what()
    bad_method,
    bad_request,

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
    ServerError(ErrCode ec);
//    ServerError(std::exception& ex, ErrCode ec = ErrCode::std_exception)
//    {
//    }

    virtual ErrCode ec() const { return ec_; }
    virtual http::status status() const { return GetInfo(ec_).status; }
    virtual std::string_view message() const { return GetInfo(ec_).msg; }

 protected:
    ErrCode ec_;

    virtual ErrInfo GetInfo(ErrCode ec) const;
};

class ApiError : public ServerError {
 private:
    using FieldValueMap = std::unordered_map<http::field, std::string>;

 public:
    using ServerError::ServerError;
    //TODO: add optional message to override one in error

    std::string print_json() const;
    const FieldValueMap& http_fields() const { return additional_error_http_fields_; }

    template<typename Val, typename... Vals>
    ApiError& AddHttpField(http::field&& field, Val&& val, Vals&&... other_vals);

 private:
    FieldValueMap additional_error_http_fields_;
    ErrInfo GetInfo(ErrCode ec) const override;
};

template<typename Val, typename... Vals>
ApiError& ApiError::AddHttpField(http::field&& field, Val&& val, Vals&&... other_vals) {
    std::stringstream ss;
    //concat args into a single string
    ss << std::forward<Val>(val);
    ((ss << ", " << http::to_string(std::forward<Vals>(other_vals))), ...);

    additional_error_http_fields_[std::move(field)] = ss.str();
    return *this;
}

class FileError : public ServerError {
 public:
    using ServerError::ServerError;
};


//===================================================================
//======================= Common =======================
// Определить MIME-тип из расширения запрашиваемого файла
std::string_view ParseMimeType(const std::string_view& path);

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body,
                                  unsigned http_version, bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML);

http::response<http::file_body> MakeResponseFromFile(const char* file_path, unsigned http_version, bool keep_alive);

//===================================================================
//======================= Async Ticker ==============================
class Ticker : public std::enable_shared_from_this<Ticker> {
 public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(model::TimeMs delta)>;

    // Функция handler будет вызываться внутри strand с интервалом period
    Ticker(Strand strand, std::chrono::milliseconds period, Handler handler);
    void Start();

 private:
    void ScheduleTick();
    void OnTick(sys::error_code ec);

    using Clock = std::chrono::steady_clock;
    Strand strand_;
    std::chrono::milliseconds period_;
    net::steady_timer timer_{strand_};
    Handler handler_;
    std::chrono::steady_clock::time_point last_tick_;
};

//===================================================================
//======================= Api Request Handler =======================

class ApiHandler : public std::enable_shared_from_this<ApiHandler> {
 public:
    using Strand = net::strand<net::io_context::executor_type>;
    ApiHandler(Strand api_strand, std::shared_ptr<app::GameInterface> game_app, model::TimeMs tick_period);

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

    bool use_http_tick_debug_ = false;
    Strand strand_;
    std::shared_ptr<app::GameInterface> game_app_;
    std::shared_ptr<Ticker> ticker_;

    std::string_view ExtractMapId(std::string_view uri) const;
    static std::pair<std::string, std::string> ExtractMapIdPlayerName (const std::string& request_body);

    std::string ExtractToken(const auto& request) const;
    app::PlayerPtr AuthorizePlayer(const auto& request) const;

    static bool RemoveIfHasPrefix(std::string_view prefix, std::string_view& uri);

    StringResponse HandleApiRequest(const StringRequest& req);

    StringResponse ReportApiError(const ApiError& err, unsigned version, bool keep_alive) const;
    StringResponse ReportApiError(unsigned version, bool keep_alive, std::string_view msg = ""sv) const;

    template<typename... Args>
    void CheckHttpMethod(const http::verb& received, Args&&... allowed) const;
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
    catch(std::exception& ex) {
        //TODO: Print exception msg
        send(ReportApiError(version, keep_alive));
    }
}

template<typename... Args>
void ApiHandler::CheckHttpMethod(const http::verb& received, Args&&... allowed) const {
    if(((received == allowed) || ...)) {
        //no throw if at least one match
        return;
    }
    throw ApiError(ErrCode::bad_method).AddHttpField(http::field::allow, std::forward<Args>(allowed)...);
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

    RequestHandler(fs::path root, Strand api_strand, std::shared_ptr<app::GameInterface> game_app, model::TimeMs tick_period);

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

/**
 * Изменение в протоколе взаимодействия с клиентом
В ответе на запрос к /api/v1/game/state теперь нужно отдавать содержимое рюкзака игроков. Для этого в информацию об игроке добавьте поле bag. Его тип — массив, содержащий информацию о собранных предметах. Информация задаётся в виде объекта со следующими полями:
id (целое число) — идентификатор предмета. Совпадает с тем идентификатором, который имел предмет до того, как его нашли.
type (целое число) — тип предмета. Тип также не должен меняться при подборе.
Следует передавать предметы в том порядке, в котором они были собраны. Вот пример ответа сервера:
{
   "players":{
      "13":{
         "pos":[10.5,3.8],
         "speed":[0.5, 0],
         "dir":"L",
         "bag":[
            {"id":9, "type":4},
            {"id":8, "type":4}
         ]
      }
   },
   "lostObjects":{
      "11":{
         "type":3,
         "pos":[13.2,17.2]
      }
   }
}
 */