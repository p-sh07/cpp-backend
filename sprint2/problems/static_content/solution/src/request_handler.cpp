#include "request_handler.h"

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
        if(iequals(ext, ".htm"sv )) return ContentType::TEXT_HTML;
        if(iequals(ext, ".html"sv)) return ContentType::TEXT_HTML;
        if(iequals(ext, ".css"sv )) return ContentType::TEXT_CSS;
        if(iequals(ext, ".txt"sv )) return ContentType::TEXT_PLAIN;
        if(iequals(ext, ".js"sv  )) return ContentType::TEXT_JS;

        //App
        if(iequals(ext, ".json"sv)) return ContentType::APP_JSON;
        if(iequals(ext, ".xml"sv )) return ContentType::APP_XML;

        //Image
        if(iequals(ext, ".png"sv )) return ContentType::IMG_PNG;
        if(iequals(ext, ".jpg"sv )) return ContentType::IMG_JPG;
        if(iequals(ext, ".jpe"sv )) return ContentType::IMG_JPG;
        if(iequals(ext, ".jpeg"sv)) return ContentType::IMG_JPG;
        if(iequals(ext, ".gif"sv )) return ContentType::IMG_GIF;
        if(iequals(ext, ".bmp"sv )) return ContentType::IMG_BMP;
        if(iequals(ext, ".ico"sv )) return ContentType::IMG_ICO;
        if(iequals(ext, ".tiff"sv)) return ContentType::IMG_TIF;
        if(iequals(ext, ".tif"sv )) return ContentType::IMG_TIF;
        if(iequals(ext, ".svg"sv )) return ContentType::IMG_SVG;
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

    http::response<http::file_body> MakeResponseFromFile(const char* file_path, unsigned http_version, bool keep_alive) {
        using namespace http;

        response<file_body> res;
        res.version(http_version);
        res.keep_alive(keep_alive);
        res.result(status::ok);
        res.insert(field::content_type, ParseMimeType(file_path));

        file_body::value_type file;

        if (sys::error_code ec; file.open(file_path, beast::file_mode::read, ec), ec) {
            //TODO: Failed to open file;
        }

        res.body() = std::move(file);
        res.prepare_payload();

        return res;
    }

    std::optional<model::Map::Id> ExtractMapId(std::string_view uri) {
        //If contains pref and has a map id, remove "/api/v1/maps/"
        if(uri.starts_with(MAP_LIST_PREFIX) && MAP_LIST_PREFIX.size() + 1 < uri.size()) {
            uri.remove_prefix(MAP_LIST_PREFIX.size() + 1);
            return model::Map::Id(std::string(uri));
        }
        return std::nullopt;
    }

    // Возвращает true, если каталог p содержится внутри base_path.
    bool IsSubPath(fs::path path, fs::path base) {
        // Приводим оба пути к каноничному виду (без . и ..)
        path = fs::weakly_canonical(path);
        base = fs::weakly_canonical(base);

        // Проверяем, что все компоненты base содержатся внутри path
        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
            if (p == path.end() || *p != *b) {
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

        for(size_t pos = 0; pos < url.size(); ) {
            if(url[pos] == '%') {
                //decode
                char decoded = std::stoul(std::string(url.substr(pos+1, 2)), nullptr, 16);
                path_string.push_back(decoded);
                pos += 3;
            } else {
                path_string.push_back(url[pos++]);
            }
        }
        std::cerr << "Converted: [" << url << "] to [" << path_string << "]\n";
        return path_string;
    }
} // namespace http_handler
