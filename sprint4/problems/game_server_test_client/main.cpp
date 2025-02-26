
//------------------------------------------------------------------------------
//
// Example: HTTP client, synchronous
//
//------------------------------------------------------------------------------

//example_http_client

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <random>
#include <vector>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

using namespace std::literals;

using Request = http::request<http::string_body>;
using Tokens = std::vector<std::string>;

Request MakeJoinRequest(const std::string& map, const std::string& player_name) {
    auto const target = "/api/v1/game/join";
    int version = 11;
    http::request<http::string_body> req{http::verb::post, target, version};

    std::string body = "{\"mapId\":\"" + map + "\",\"userName\": \"" + player_name +"\" }";

    req.set(http::field::content_type, "application/json"sv);
    req.content_length(body.size());
    req.body() = body;

    return req;
}

Request MakeMoveRequest(const std::string& token, std::string move_char) {
    auto const target = "/api/v1/game/player/action";
    int version = 11;
    http::request<http::string_body> req{http::verb::post, target, version};

    std::string body = "{\"move\":\"" + move_char + "\"}";

    req.set(http::field::authorization, "Bearer " + token);
    req.set(http::field::content_type, "application/json"sv);
    req.content_length(body.size());
    req.body() = body;

    return req;
}

Request MakeTickRequest(size_t tick_time) {
    auto const target = "/api/v1/game/tick";
    int version = 11;
    http::request<http::string_body> req{http::verb::post, target, version};

    std::string body = "{\"timeDelta\":" + std::to_string(tick_time) + "}";

    req.set(http::field::content_type, "application/json"sv);
    req.content_length(body.size());
    req.body() = body;

    return req;
}

Request MakeRecordsRequest() {
    auto const target = "/api/v1/game/records";
    int version = 11;
    http::request<http::string_body> req{http::verb::get, target, version};

    req.set(http::field::content_type, "application/json"sv);

    return req;
}

std::string ExtractToken(std::string_view sv) {
    std::string_view tokenStart = "\"authToken\":\"";
    std::string_view tokenStart_2 = "\"authToken\": \"";
    const auto tokenLen = 32u;

    if (auto startPos = sv.find(tokenStart); startPos != std::string_view::npos) {
        return std::string{sv.substr(startPos + tokenStart.size(), tokenLen)};
    }
    else if(auto StartPos2 = sv.find(tokenStart_2); StartPos2 != std::string_view::npos) {
        return std::string{sv.substr(StartPos2 + tokenStart.size(), tokenLen)};
    }
    std::cerr << "Unable to extrect token" << std::endl;
    return "";
}

Tokens JoinPlayers(beast::tcp_stream& stream, const std::string& map, size_t num_players, const std::string& name_prefix = "player") {
    std::vector<std::string> tokens;

    for(size_t i = 0; i < num_players; ++i) {
        auto player_name = name_prefix + std::to_string(i);
        http::write(stream, MakeJoinRequest(map, player_name));

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        tokens.emplace_back(ExtractToken(res.body()));
    }
    return tokens;
}

void RandomMovePlayers(beast::tcp_stream& stream, const Tokens& player_tokens) {
    //move all players by token
    for(const auto& token : player_tokens) {
        const auto move_chars = "RLUD"s;
        const std::string random_mv_char{move_chars[std::rand() % move_chars.size()]};

        http::write(stream, MakeMoveRequest(token, random_mv_char));

        // no need to read?
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        if(res.result() != http::status::ok) {
            assert(false);
            throw std::runtime_error("no ok from server"s);
        }
    }
}

void StopAllPlayers(beast::tcp_stream& stream, const Tokens& player_tokens) {
    //move all players by token
    for(const auto& token : player_tokens) {

        http::write(stream, MakeMoveRequest(token, ""));

        // no need to read?
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        if(res.result() != http::status::ok) {
            assert(false);
            throw std::runtime_error("no ok from server"s);
        }
    }
}

void Tick(beast::tcp_stream& stream, size_t tick_time) {
    //choose random tick time
    http::write(stream, MakeTickRequest(tick_time));

    // no need to read?
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    if (res.result() != http::status::ok) {
        throw std::runtime_error("no ok from server"s);
    }
}

void RandomTick(beast::tcp_stream& stream, size_t max_tick) {
    //choose random tick time
    const size_t random_tick_time = (100u + std::rand() % max_tick);
    std::cerr << "Tick rnd: " << random_tick_time << '\n';

    Tick(stream, random_tick_time);
}

void GetScores(beast::tcp_stream& stream) {
    //choose random tick time
    http::write(stream, MakeRecordsRequest());

    // no need to read?
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    if (res.result() != http::status::ok) {
        throw std::runtime_error("no ok from server"s);
    }
}

void GetRecords(beast::tcp_stream& stream) {
}

//=============================================//
//=================== PARAMS ===================//
static const auto MAP_NAME = "map3"s;
static const auto MAP_TOWN = "map3"s;
static const auto MAP_MAP1 = "map1"s;
static const auto MAP_MAP3 = "map3"s;
static const auto NUM_PLAYERS = 10u;
static const auto NUM_ITERATIONS = 200u;
static const auto MAX_TICK_TIME = 10000u;
static const auto RETIRE_TIME = 15000u;

//=============================================//

void RunTestOldTribes(beast::tcp_stream& stream) {
    //Send random move requests for all players, followed by a random tick, repeat Num times
    //Old tribe
    auto elder_tribe = JoinPlayers(stream, MAP_NAME, NUM_PLAYERS, "Old");
    for(int n = 0; n < NUM_ITERATIONS; ++n) {
        RandomMovePlayers(stream, elder_tribe);
        RandomTick(stream, MAX_TICK_TIME);
    }
    Tick(stream, RETIRE_TIME / 2u);
    StopAllPlayers(stream, elder_tribe);

    //young tribe
    auto young_tribe = JoinPlayers(stream, MAP_NAME, NUM_PLAYERS, "Young");
    for(int n = 0; n < NUM_ITERATIONS; ++n) {
        RandomMovePlayers(stream, young_tribe);
        RandomTick(stream, MAX_TICK_TIME);
    }
    Tick(stream, RETIRE_TIME / 2u);
}

void RunTestTwoSequential(beast::tcp_stream& stream) {
    //Send random move requests for all players, followed by a random tick, repeat Num times
    //Old tribe
    auto red_foxes = JoinPlayers(stream, MAP_NAME, 50, "red_fox");
    for(int n = 0; n < 35; ++n) {
        RandomMovePlayers(stream, red_foxes);
        RandomTick(stream, MAX_TICK_TIME);
    }
    StopAllPlayers(stream, red_foxes);
    Tick(stream, RETIRE_TIME);
    GetScores(stream);

    //orange_racoons
    auto orange_raccoons = JoinPlayers(stream, MAP_NAME, 50, "or_racc");
    for(int n = 0; n < 35; ++n) {
        RandomMovePlayers(stream, orange_raccoons);
        RandomTick(stream, MAX_TICK_TIME);
    }
    StopAllPlayers(stream, orange_raccoons);
    Tick(stream, RETIRE_TIME);
    GetScores(stream);
}

void RunTestAFewRecords(beast::tcp_stream& stream, const std::string& map_name = MAP_NAME) {
    //Send random move requests for all players, followed by a random tick, repeat Num times
    //Old tribe
    auto red_foxes = JoinPlayers(stream, map_name, 100, "a_few_records");
    for(int n = 0; n < 350; ++n) {
        RandomMovePlayers(stream, red_foxes);
        RandomTick(stream, MAX_TICK_TIME);
    }
    GetScores(stream);
    StopAllPlayers(stream, red_foxes);
    Tick(stream, RETIRE_TIME);

    GetScores(stream);
}

void RunTestAHundredPlusRecords(beast::tcp_stream& stream, const std::string& map_name = MAP_NAME) {
    //Send random move requests for all players, followed by a random tick, repeat Num times
    //Old tribe
    auto red_foxes = JoinPlayers(stream, map_name, 150, "legion");
    for(int n = 0; n < 35; ++n) {
        RandomMovePlayers(stream, red_foxes);
        RandomTick(stream, MAX_TICK_TIME);
    }
    StopAllPlayers(stream, red_foxes);
    Tick(stream, RETIRE_TIME);
    GetScores(stream);
}

//================ Server Testing ============//
int main(int argc, char** argv)
{
    try
    {
        auto const host = "localhost";
        auto const port = "80";

        // The io_context is required for all I/O
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        stream.connect(results);

        while (true) {
            RunTestAFewRecords(stream);
            RunTestAHundredPlusRecords(stream);
        }

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        //
        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};

        // If we get here then the connection is closed gracefully
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::cerr << "Sent all requests"s << std::endl;
    return EXIT_SUCCESS;
}

