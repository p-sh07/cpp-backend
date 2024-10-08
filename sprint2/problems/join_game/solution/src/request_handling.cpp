#include "request_handling.h"

namespace http_handler {

// Определить MIME-тип из расширения запрашиваемого файла
std::string_view ParseMimeType(const std::string_view& path) {
    using beast::iequals;

    //Use octet-stream for empty or unknown:
    auto dot_pos = path.rfind('.');
    if(path.npos == dot_pos) {
        return ContentType::APP_OTHER;
    }
    auto ext = path.substr(dot_pos);

    //Text
    if(iequals(ext, ".htm"sv)) return ContentType::TEXT_HTML;
    if(iequals(ext, ".html"sv)) return ContentType::TEXT_HTML;
    if(iequals(ext, ".css"sv)) return ContentType::TEXT_CSS;
    if(iequals(ext, ".txt"sv)) return ContentType::TEXT_PLAIN;
    if(iequals(ext, ".js"sv)) return ContentType::TEXT_JS;

    //App
    if(iequals(ext, ".json"sv)) return ContentType::APP_JSON;
    if(iequals(ext, ".xml"sv)) return ContentType::APP_XML;

    //Image
    if(iequals(ext, ".png"sv)) return ContentType::IMG_PNG;
    if(iequals(ext, ".jpg"sv)) return ContentType::IMG_JPG;
    if(iequals(ext, ".jpe"sv)) return ContentType::IMG_JPG;
    if(iequals(ext, ".jpeg"sv)) return ContentType::IMG_JPG;
    if(iequals(ext, ".gif"sv)) return ContentType::IMG_GIF;
    if(iequals(ext, ".bmp"sv)) return ContentType::IMG_BMP;
    if(iequals(ext, ".ico"sv)) return ContentType::IMG_ICO;
    if(iequals(ext, ".tiff"sv)) return ContentType::IMG_TIF;
    if(iequals(ext, ".tif"sv)) return ContentType::IMG_TIF;
    if(iequals(ext, ".svg"sv)) return ContentType::IMG_SVG;
    if(iequals(ext, ".svgz"sv)) return ContentType::IMG_SVG;

    //Audio
    if(iequals(ext, ".mp3"sv)) return ContentType::AUDIO_MP3;

    //Other
    return ContentType::APP_OTHER;
}

StringResponse MakeStringResponse(http::status status, std::string_view body,
                                  unsigned http_version, bool keep_alive,
                                  std::string_view content_type) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

http::response<http::file_body> MakeResponseFromFile(const char*file_path, unsigned http_version, bool keep_alive) {
    using namespace http;

    response<file_body> res;
    res.version(http_version);
    res.keep_alive(keep_alive);
    res.result(status::ok);
    res.insert(field::content_type, ParseMimeType(file_path));

    file_body::value_type file;

    if(sys::error_code ec; file.open(file_path, beast::file_mode::read, ec), ec) {
        //TODO: Failed to open file;
    }

    res.body() = std::move(file);
    res.prepare_payload();

    return res;
}

// Возвращает true, если каталог p содержится внутри base_path.
bool IsSubPath(fs::path path, fs::path base) {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for(auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if(p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

//Конвертирует URL-кодированную строку в путь
fs::path ConvertFromUrl(std::string_view url) {
    if(url.empty()) {
        return {};
    }

    std::string path_string;

    for(size_t pos = 0; pos < url.size();) {
        if(url[pos] == '%') {
            //decode
            char decoded = std::stoul(std::string(url.substr(pos + 1, 2)), nullptr, 16);
            path_string.push_back(decoded);
            pos += 3;
        } else {
            path_string.push_back(url[pos++]);
        }
    }
    return path_string;
}

//======================= Api Request Handler ======================
ApiHandler::ApiHandler(Strand api_strand, std::shared_ptr<app::GameInterface> game_app)
    : strand_(api_strand)
    , game_app_(std::move(game_app)) {
}

std::string_view ApiHandler::ExtractMapId(std::string_view uri) {
    //If contains pref and has a map id, remove "/api/v1/maps/"
    if(uri.starts_with(map_list_prefix_) && map_list_prefix_.size() + 1 < uri.size()) {
        uri.remove_prefix(map_list_prefix_.size() + 1);
        return uri;
    }
    return {};
}

StringResponse ApiHandler::HandleApiRequest(const StringRequest& req) {
    auto to_html = [&](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, req.version(),
                                  req.keep_alive(), ContentType::APP_JSON);
    };

    std::string_view request_uri = req.target();

    // ->Request for map list
    if(request_uri == map_list_prefix_) {
        return to_html(http::status::ok,
                       json_loader::PrintMapList(game_app_->ListAllMaps()));
    }

    // ->Request a map - contains full prefix for map list
    if(auto map_id = ExtractMapId(request_uri); !map_id.empty()) {
        if(auto map = game_app_->GetMap(map_id)) {
            return to_html(http::status::ok,
                           json_loader::PrintMap(*map));
        }
        throw ApiError(ErrCode::map_not_found);
    }

  // ->Game interaction
    // ->Join game
    if(request_uri == join_game_prefix_) {
        if(req.method() != http::verb::post) {
            throw ApiError(ErrCode::join_game_bad_method);
        }

        auto join_map_player = ExtractMapIdAndPlayerName(req.body());

        //Requested map doesnt exist
        //TODO: Refactor error throw?
        if(!game_app_->GetMap(join_map_player.first)) {
            throw ApiError(ErrCode::map_not_found);
        }

        auto player_id_token = game_app_->JoinGame(join_map_player.first, join_map_player.second);

        json::object json_body = {
            {"playerId", player_id_token.first},
            {"authToken", **player_id_token.second},
        };

        StringResponse resp = to_html(http::status::ok, serialize(json_body));
        //TODO: Refactor no-cache into to_html?
        resp.set(http::field::cache_control, "no-cache");

        return resp;
    }

    // ->Get player list
    if(request_uri == player_list_prefix_) {
        if(req.method() != http::verb::get && req.method() != http::verb::head) {
            throw ApiError(ErrCode::player_list_bad_method);
        }

        const auto token = ExtractAuthToken(req);

        auto player = game_app_->FindPlayerByToken(token);
        if(!player) {
            throw ApiError(ErrCode::unknown_token);
        }

        auto json_body = MakePlayerListJson(game_app_->GetPlayerList(player));

        StringResponse resp = to_html(http::status::ok, serialize(json_body));
        resp.set(http::field::cache_control, "no-cache");

        return resp;
    }

    // *Error: Bad request
    throw ApiError(ErrCode::bad_request);
}

json::object ApiHandler::MakePlayerListJson(const std::vector<app::PlayerPtr>& plist) const {
    json::object result;
    for(const auto& p_ptr : plist) {
        result.emplace(std::to_string(p_ptr->GetId()),
                        json::object{
                            {"name", std::string(p_ptr->GetDog()->GetName())}
                        }
        );
    }
    return result;
}

StringResponse ApiHandler::ReportApiError(const ApiError& err, unsigned version, bool keep_alive) const {
    auto resp = MakeStringResponse(err.status(), err.print_json(), version,
                       keep_alive, ContentType::APP_JSON);
    resp.set(http::field::cache_control, "no-cache"s);

    if(err.ec() == ErrCode::join_game_bad_method) {
        resp.set(http::field::allow, "POST"s);
    }
    if(err.ec() == ErrCode::player_list_bad_method) {
        resp.set(http::field::allow, "GET, HEAD"s);
    }
    return resp;
}

StringResponse ApiHandler::ReportApiError(unsigned version, bool keep_alive) const {
    return MakeStringResponse(http::status::unknown, "Api handling error occured"sv, version, keep_alive);
}

//======================= File Request Handler ======================
FileHandler::FileHandler(fs::path root)
    : root_(std::move(root)) {
}

FileHandler::FileRequestResult FileHandler::HandleFileRequest(const StringRequest& req) const {
    //String response for api requests
    auto to_html = [&](http::status status, std::string_view text, std::string_view content_type) {
        return MakeStringResponse(status, text, req.version(),
                                  req.keep_alive(), content_type);
    };

    std::string_view request_uri = req.target();

    //Try filesystem request
    fs::path requested_file;
    if(request_uri == "/"sv || request_uri == "/index.html"sv) {
        //special case, return index.html
        requested_file = root_ / fs::path("index.html"s);
    } else {
        //remove leading '/' to ensure correct splicing
        if(!request_uri.empty() && request_uri[0] == '/') {
            request_uri.remove_prefix(1);
        }
        requested_file = fs::weakly_canonical(root_ / ConvertFromUrl(request_uri));
    }

    //Error if requested file does not exist
    if(!fs::exists(requested_file)) {
        return to_html(http::status::not_found,
                       "File not found"s,
                       ContentType::TEXT_PLAIN);;
    }
        //File outside filesystem root
    else if(!IsSubPath(requested_file, root_)) {
        return to_html(http::status::bad_request,
                       "Access denied"s,
                       ContentType::TEXT_PLAIN);
    }

    //File found, try to open & send
    return MakeResponseFromFile(requested_file.c_str(), req.version(), req.keep_alive());
}

StringResponse FileHandler::ReportFileError(const FileError& err, unsigned version, bool keep_alive) const {
    //TODO: Handle error
    return {};
}

StringResponse FileHandler::ReportFileError(unsigned version, bool keep_alive) const {
    //TODO: Handle error
    return {};
}

//======================= Request Handling Interface ==========================
StringResponse RequestHandler::ReportServerError(const ServerError& err, unsigned version, bool keep_alive) const {
    auto error_report = MakeStringResponse(err.status(), err.what(), version, keep_alive);
    if(err.status() == http::status::method_not_allowed) {
        error_report.set(http::field::allow, "GET, HEAD, POST"sv);
    }

    //NB: Can add other cases:


    return error_report;
}

StringResponse RequestHandler::ReportServerError(unsigned version, bool keep_alive) const {
    return MakeStringResponse(http::status::unknown, "Request handling error occured"sv, version, keep_alive);
}
} // namespace http_handler
