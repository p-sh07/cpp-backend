#pragma once
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
// #include <boost/serialization/>

#include "application.h"
#include "model.h"

#include <iostream>
#include <fstream>
#include <ranges>


namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}
}  // namespace geom

// namespace detail {
// template <typename Archive>
// void serialize(Archive& ar, TokenTag& obj, [[maybe_unused]] const unsigned version) {
//     ar& obj;
// }
// }  // namespace detail

namespace model {
template <typename Archive>
void serialize(Archive& ar, Map::Id& obj, [[maybe_unused]] const unsigned version) {
    ar&(*obj);
}

template <typename Archive>
void serialize(Archive& ar, LootItemInfo& obj, [[maybe_unused]] const unsigned version) {
    ar&(obj.id);
    ar&(obj.type);
}

template <typename Archive>
void serialize(Archive& ar, TimeMs& obj, [[maybe_unused]] const unsigned version) {
    ar& obj;
}

}  // namespace model


namespace app {
// template <typename Archive>
// void serialize(Archive& ar, Session::Id& sess_id, [[maybe_unused]] const unsigned version) {
//     ar& sess_id;
// }

template <typename Archive>
void serialize(Archive& ar, Token& obj, [[maybe_unused]] const unsigned version) {
    ar&(*obj);
}
}  // namespace app

namespace serialization {
namespace range = std::ranges;
using namespace std::literals;

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog);

    [[nodiscard]] model::Dog Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& pos_;
        ar& width_;
        ar& speed_;
        ar& direction_;
        ar& name_;
        ar& bag_capacity_;
        ar& score_;
        ar& bag_content_;
        ar& ingame_time_;
        ar& inactive_time_;
    }

private:
    model::Dog::Id id_ {0u};
    model::Point2D pos_;
    double width_;
    model::Speed speed_;
    model::Direction direction_ {model::Direction::NORTH};;
    std::string name_ {""};
    size_t bag_capacity_ {0u};
    model::Score score_ {0u};
    model::Dog::BagContent bag_content_;
    int64_t ingame_time_;
    int64_t inactive_time_;
};

struct LootItemRepr {
    LootItemRepr() = default;

    explicit LootItemRepr(const model::LootItem& item);

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id;
        ar& type;
        ar& pos;
    }

    model::LootItem::Id id {0u};
    model::LootItem::Type type {0u};
    model::Point2D pos;
};

class SessionRepr {
public:
    SessionRepr() = default;

    explicit SessionRepr(const app::Session& session);

    app::Session Restore(const app::GamePtr& game) const;

    app::Session::Id GetId() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& map_id_content_;
        ar& dog_reprs_;
        ar& loot_item_reprs_;
    }

private:
    size_t id_ {0u};
    std::string map_id_content_ {""s};

    std::vector<DogRepr> dog_reprs_;
    std::vector<LootItemRepr> loot_item_reprs_;
};

class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(const app::Player& player, app::Token token);

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& session_id_;
        ar& dog_id_;
        ar& token_content_;
    }

    app::Player::Id GetId() const;
    app::Session::Id GetSessionId() const;
    model::Dog::Id GetDogId() const;
    app::Token GetToken() const;

private:
    app::Player::Id id_;
    app::Session::Id session_id_;
    model::Dog::Id dog_id_;
    std::string token_content_;
};

class PsmRepr {
public:
    PsmRepr() = default;

    explicit PsmRepr(const app::PlayerSessionManager& psm);

    app::PlayerSessionManager Restore(const app::GamePtr& game) const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& session_reprs_;
        ar& player_reprs_;
    }

private:
    std::vector<SessionRepr> session_reprs_;
    std::vector<PlayerRepr> player_reprs_;
};


//=================================================
//================ Serializer =====================
namespace fs = std::filesystem;
namespace arch = boost::archive;
class StateSerializer : public app::ApplicationListener {
public:
    StateSerializer(fs::path save_file, bool enable_periodic_backup, int64_t save_period);
    ~StateSerializer() override = default;

    void OnTick(model::TimeMs delta_t, const app::PlayerSessionManager& psm) override;
    void SaveGameState(const app::PlayerSessionManager& psm) const;

    app::PlayerSessionManager Restore(app::GamePtr game) const override;

private:
    fs::path save_temp_ = "temp"s;
    fs::path save_file_;
    fs::path save_dir_;

    bool enable_periodic_backup_ = false;
    model::TimeMs save_period_;

    model::TimeMs time_since_last_save_ {0u};
};


}  // namespace serialization
