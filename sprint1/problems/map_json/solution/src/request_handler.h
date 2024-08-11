#pragma once
#include "http_server.h"
#include "model.h"

#include <boost/json/detail/index_sequence.hpp>
#include <ranges>
#include <sstream>
#include <string>

#include "json_loader.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace view = std::views;
namespace json = boost::json;

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
  constexpr static std::string_view APP_JSON = "application/json"sv;
  // При необходимости внутрь ContentType можно добавить и другие типы контента
};

// Создаёт StringResponse с заданными параметрами
StringResponse
MakeStringResponse(http::status status, std::string_view body,
                   unsigned http_version, bool keep_alive,
                   std::string_view content_type = ContentType::APP_JSON);

class RequestHandler {
public:
  explicit RequestHandler(model::Game &game) : game_{game} {}

  RequestHandler(const RequestHandler &) = delete;
  RequestHandler &operator=(const RequestHandler &) = delete;

  template <typename Body, typename Allocator, typename Send>
  void operator()(http::request<Body, http::basic_fields<Allocator>> &&req,
                  Send &&send);

private:
  model::Game &game_;
  const std::string_view api_prefix{"/api/"sv};
  const std::string_view maps_list_uri{"/api/v1/maps"sv};

  std::vector<std::string> ParseUri(std::string_view uri);
};

template <typename Body, typename Allocator, typename Send>
void RequestHandler::operator()(
    http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send) {
  // Обработать запрос request и отправить ответ, используя send
  auto text_response = [&](http::status status, std::string_view text) {
    return MakeStringResponse(status, text, req.version(), req.keep_alive());
  };

  auto request_type = req.method();

  // Unknown method
  if (request_type != http::verb::get) {
    auto response =
        text_response(http::status::method_not_allowed, "Invalid method"sv);
    response.set(http::field::allow, "GET"sv);

    send(response);
    return;
  }

  // URI error
  std::string_view tg_full = req.target();
  auto uri_tokens = ParseUri(tg_full);

  if (uri_tokens.empty()) {
    // empty err
  }

  // starts with "/api/"
  if (tg_full.rfind(api_prefix, 0) == 0) {

    // contains "/api/v1/maps"
    if (uri_tokens.size() > 2 && tg_full.rfind(maps_list_uri, 0) == 0) {
      // /api/v1/maps/{id-карты}
      if (uri_tokens.size() > 3) {
        // return Map json
        const auto map = game_.FindMap(model::Map::Id(uri_tokens[3]));
        if (!map) {
          const std::string body = "{\n  \"code\": \"mapNotFound\"\n  "
                                   "\"message\": \"Map not found\"\n}";
          send(text_response(http::status::not_found, body));
          return;
        }

        // Map found
        send(text_response(http::status::found,
          json_loader::PrintMap(*map)));
        return;
      }

      // Send map list
      send(text_response(http::status::found,
        json_loader::PrintMapList(game_)));
      return;
    }

    // contains only "/api/"
    std::string body =
        "{\n  \"code\": \"badRequest\",\n  \"message\": \"Bad request\"\n}"s;
    send(text_response(http::status::bad_request, body));
            return;
        }

        //Error handling
        std::string body = "{\n  \"code\": \"notFound\",\n  \"message\": \"Not found\"\n}"s;
        send(text_response(http::status::not_found, body));

    }

}// namespace http_handler
