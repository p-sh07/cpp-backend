//
// Created by Pavel on 22.08.24.
//
#pragma once
#include <deque>
#include <memory>
#include <random>
#include <unordered_map>

#include "app_util.h"
#include "model.h"

namespace detail {
struct TokenTag {};
}  // namespace detail

namespace app {
//static constexpr char MOVE_CMD_STOP = '!';
static constexpr char MOVE_CMD_ERR = '%';

using model::Game;
using model::Map;
using model::Dog;
using model::Session;

using SessionPtr = Session*;
using DogPtr = Dog*;

class Player {
 public:
    Player(size_t id, SessionPtr session, DogPtr dog);

    size_t GetId() const { return id_; }
    Dog* GetDog() { return dog_; }
    Session* GetSession() { return session_; }

    const Dog* GetDog() const { return dog_; }
    const Session* GetSession() const { return session_; }
    const Map* GetMap() const { return GetSession()->GetMap(); }

 private:
    const size_t id_ = 0;
    SessionPtr session_;
    DogPtr dog_;
};

using Token = util::Tagged<std::string, detail::TokenTag>;

struct TokenHasher {
    size_t operator()(const Token& token) const;
};

using GamePtr = std::shared_ptr<Game>;
using PlayerPtr = Player*;
using TokenPtr = const Token*;

using PlayerId = size_t;
using DogId = size_t;

class Players {
 public:
    explicit Players(const GamePtr& game);
    explicit Players(GamePtr&& game);

    Player& Add(Dog* dog, Session* session);

    PlayerPtr GetByToken(const Token& token) const;
    PlayerPtr GetByMapDogId(const  Map::Id& map_id, size_t dog_id) const;
    TokenPtr GetToken(const Player& player) const;

    std::vector<PlayerPtr> GetSessionPlayerList(const Player& player);

 private:
    GamePtr game_;
    std::deque<Player> players_;
    PlayerId next_player_id_ = 0;

    using TokenIt = std::unordered_map<Token, PlayerId>::iterator;

    //Indices for search
    std::unordered_map<Token, PlayerId, TokenHasher> token_to_player_;
    std::unordered_map<PlayerId, TokenIt> player_to_token_;

    using MapDogIdToPlayer = std::unordered_map< Map::Id, std::unordered_map<DogId, PlayerId>,  util::TaggedHasher<Map::Id>>;
    MapDogIdToPlayer map_dog_id_to_player_;

    Token GenerateToken() const;
};

struct JoinGameResult {
    PlayerId player_id;
    const Token* token;
};

class GameInterface {
 public:
    GameInterface(GamePtr& game_ptr);

    //use cases
    const Map* GetMap(std::string_view map_id) const;
    const Game::Maps& ListAllMaps() const;

    void MovePlayer(PlayerPtr player, const char move_command);
    void AdvanceGameTime(model::Time delta_t);

    JoinGameResult JoinGame(std::string_view map_id_str, std::string_view player_dog_name);
    PlayerPtr FindPlayerByToken(const Token& token) const;
    std::vector<PlayerPtr> GetPlayerList(PlayerPtr player);

 private:
    GamePtr game_;
    Players players_;

};

class Ticker : public std::enable_shared_from_this<Ticker> {
 public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(std::chrono::milliseconds delta)>;

    // Функция handler будет вызываться внутри strand с интервалом period
    Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
        : strand_{strand}
        , period_{period}
        , handler_{std::move(handler)} {
    }

    void Start() {
        net::dispatch(strand_, [self = shared_from_this()] {
            last_tick_ = Clock::now();
            self->ScheduleTick();
        });
    }

 private:
    void ScheduleTick() {
        assert(strand_.running_in_this_thread());
        timer_.expires_after(period_);
        timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
            self->OnTick(ec);
        });
    }

    void OnTick(sys::error_code ec) {
        using namespace std::chrono;
        assert(strand_.running_in_this_thread());

        if (!ec) {
            auto this_tick = Clock::now();
            auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
            last_tick_ = this_tick;
            try {
                handler_(delta);
            } catch (...) {
            }
            ScheduleTick();
        }
    }

    using Clock = std::chrono::steady_clock;

    Strand strand_;
    std::chrono::milliseconds period_;
    net::steady_timer timer_{strand_};
    Handler handler_;
    std::chrono::steady_clock::time_point last_tick_;
};

net::io_context ioc;
...
// Объект Application содержит сценарии использования
Application app{/* ... */};

// strand, используемый для доступа к API
auto api_strand = net::make_strand(ioc);

// Настраиваем вызов метода Application::Tick каждые 50 миллисекунд внутри strand
auto ticker = std::make_shared<Ticker>(api_strand, 50ms,
                                       [&app](std::chrono::milliseconds delta) { app.Tick(delta); }
);
ticker->Start();
...


}
