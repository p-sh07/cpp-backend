#include "request_handler.h"

namespace http_handler {

    model::Map::Id GetMapId(std::string_view uri) {
        //remove "/api/v1/maps" + '/'
        uri.remove_prefix(MAP_LIST_PREFIX.size() + 1);
        return model::Map::Id(std::string(uri));
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
} // namespace http_handler
