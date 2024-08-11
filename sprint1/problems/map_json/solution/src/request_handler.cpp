#include "request_handler.h"

namespace http_handler {

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

std::vector<std::string> RequestHandler::ParseUri(std::string_view uri) {
  // auto tokens = uri | view::split('/') | view::transform()
  // return {tokens.begin(), tokens.end()};

  // remove leading /
  uri.remove_prefix(1);
  std::istringstream ss{std::string(uri)};

  std::vector<std::string> tokens;
  std::string token;

  while (std::getline(ss, token, '/')) {
    tokens.push_back(token);
  }
  return tokens;

  // Construct a vector of tokens from stringstream
  // return {(std::istream_iterator<std::string>(ss)),
  // std::istream_iterator<std::string>()};
}
} // namespace http_handler

/*
*#include "boost/json/src.hpp" // header-only approach
#include <iostream>
namespace json = boost::json;
using json::value_from;
using json::value_to;

static const json::value expected = json::parse(R"({
  "track": {
    "Wheels": {
      "Wheel": [
        {
          "start_pos": "10",
          "end_pos": "25"
        },
        {
          "start_pos": "22",
          "end_pos": "78"
        }
      ]
    },
    "Brakes": {
      "Brake": [
        {
        "start_pos": "10",
        "midl_pos": "25"
        }
      ]
    }
  }
})");

namespace MyLib {
    struct wheel { int start_pos, end_pos; };
    struct brake { int start_pos, midl_pos; };

    struct track {
        std::vector<wheel> wheels;
        std::vector<brake> brakes;
    };

    void tag_invoke(json::value_from_tag, json::value& jv, wheel const& w) {
        jv = {
            {"start_pos", std::to_string(w.start_pos)},
            {"end_pos", std::to_string(w.end_pos)},
        };
    }

    void tag_invoke(json::value_from_tag, json::value& jv, brake const& b) {
        jv = {
            {"start_pos", std::to_string(b.start_pos)},
            {"midl_pos", std::to_string(b.midl_pos)},
        };
    }

    void tag_invoke(json::value_from_tag, json::value& jv, track const& t) {
        jv = {{"track",
               {
                   {"Wheels", {{"Wheel", t.wheels}}},
                   {"Brakes", {{"Brake", t.brakes}}},
               }}};
    }
}

int main()
{
    MyLib::track track{{
                           {10, 25},
                           {22, 78},
                       },
                       {
                           {10, 25},
                       }};

    json::value output = json::value_from(track);
    std::cout << output << "\n";

    std::cout << expected << "\n";
    std::cout << "matching: " << std::boolalpha << (output == expected) << "\n";
}
*/

/*
#include <boost/json/src.hpp>
#include <iostream>

template <class T> void extract(boost::json::object const& obj, T& v,
boost::json::string_view key) { v = boost::json::value_to<T>(obj.at(key));
};

struct CRDApp {
    std::string type;
    std::string image;
    uint32_t    replicas;

    friend CRDApp tag_invoke(boost::json::value_to_tag<CRDApp>,
boost::json::value const& jv) { CRDApp                     app;
        boost::json::object const& obj = jv.as_object();
        extract(obj, app.type, "type");
        extract(obj, app.image, "image");
        extract(obj, app.replicas, "replicas");
        return app;
    }

    friend void tag_invoke(boost::json::value_from_tag, boost::json::value& jv,
CRDApp const& app) { jv = {{"type", app.type}, {"image", app.image},
{"replicas", app.replicas}};
    }

    auto operator<=>(CRDApp const&) const = default;
};

struct CRDSpec {
    std::string         _namespace;
    std::vector<CRDApp> apps;

    friend CRDSpec tag_invoke(boost::json::value_to_tag<CRDSpec>,
boost::json::value const& jv) { CRDSpec                    spec;
        boost::json::object const& obj = jv.as_object();
        extract(obj, spec._namespace, "namespace");
        extract(obj, spec.apps, "apps");
        return spec;
    }

    friend void tag_invoke(boost::json::value_from_tag, boost::json::value& jv,
CRDSpec const& spec) { jv = {{"namespace", spec._namespace}, {"apps",
boost::json::value_from(spec.apps)}};
    }

    auto operator<=>(CRDSpec const&) const = default;
};

int main() {
    CRDSpec const spec{"some_ns",
                       {
                           {"type1", "image1", 111},
                           {"type2", "image2", 222},
                           {"type3", "image3", 333},
                           {"type4", "image4", 444},
                           {"type5", "image5", 555},
                           {"type6", "image6", 666},
                           {"type7", "image7", 777},
                           {"type8", "image8", 888},
                           {"type9", "image9", 999},
                       }};

    auto js = boost::json::value_from(spec);
    std::cout << js << "\n";

    auto roundtrip = boost::json::value_to<CRDSpec>(js);
    std::cout << "Roundtrip " << (roundtrip == spec? "equal":"different") <<
"\n";
}
*/
