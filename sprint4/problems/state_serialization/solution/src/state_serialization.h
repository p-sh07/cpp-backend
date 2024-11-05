#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/unordered_set.hpp>
// #include <boost/serialization/
// #include <boost/serialization/

#include "application.h"
#include "model.h"

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

namespace model {
template <typename Archive>
void serialize(Archive& ar, Dog::Tag& obj, [[maybe_unused]] const unsigned version) {
    ar&(*obj);
}

template <typename Archive>
void serialize(Archive& ar, LootItem::Info& obj, [[maybe_unused]] const unsigned version) {
    ar&(obj.id);
    ar&(obj.type);
}

}  // namespace model


namespace app {
template <typename Archive>
void serialize(Archive& ar, Session& obj, [[maybe_unused]] const unsigned version) {
    // ar&(*obj);
}

template <typename Archive>
void serialize(Archive& ar, LootItem::Info& obj, [[maybe_unused]] const unsigned version) {
    ar&(obj.id);
    ar&(obj.type);
}

}  // namespace model

namespace serialization {

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(dog.GetId())
          , pos_(dog.GetPos())
          , width_(dog.GetWidth())
          , speed_(dog.GetSpeed())
          , direction_(dog.GetDirection())
          , tag_(dog.GetTag())
          , bag_capacity_(dog.GetBagCap())
          , score_(dog.GetScore())
          , bag_content_(dog.GetBag()) {
    }

    [[nodiscard]] model::Dog Restore() const {
        model::Dog dog{id_, pos_, width_, tag_, bag_capacity_};
        dog.SetSpeed(speed_);
        dog.SetDirection(direction_);
        dog.AddScore(score_);
        for (const auto& item : bag_content_) {
            if (!dog.TryCollectItem(item)) {
                throw std::runtime_error("Failed to put bag content");
            }
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& pos_;
        ar& width_;
        ar& speed_;
        ar& direction_;
        ar& tag_;
        ar& bag_capacity_;
        ar& score_;
        ar& bag_content_;
    }

private:
    model::Dog::Id id_ {0u};
    model::Point2D pos_;
    double width_;
    model::Speed speed_;
    model::Direction direction_ {model::Direction::NORTH};;
    model::Dog::Tag tag_ {""};
    size_t bag_capacity_ {0u};
    model::Score score_ {0u};
    model::Dog::BagContent bag_content_;
};

//TODO: Can use inheritance here also?
class LootItemRepr {
public:
    LootItemRepr() = default;

    explicit LootItemRepr(const model::LootItem& item)
        : id_(item.GetId())
          , type_(item.GetType())
          , pos_(item.GetPos())
          , width_(item.GetWidth()) {
    }

    app::LootItem Restore() const {
        return model::LootItem{id_, pos_, width_, type_};
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& type_;
        ar& pos_;
        ar& width_;
    }

private:
    model::LootItem::Id id_ {0u};
    model::LootItem::Type type_ {0u};
    model::Point2D pos_;
    double width_ {0.0};
};

class SessionRepr {
public:
    SessionRepr() = default;

    explicit SessionRepr(const app::Session& session)
    :
    {}

    app::Session Restore() const {

    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {

    }

private:
    const size_t id_;
    model::TimeMs time_ {0u};

    std::vector<DogRepr> dogs_;
    std::vector<LootItemRepr> loot_items_;

    model::Map::Id map_id;
};


class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(const app::Player& player)
        : id_(player.GetId())
          , session_id_(player.GetSession()->GetId())
          , dog_id_(player.GetDog()->GetId()) {
    }

    app::Player Restore() const {

    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {

    }

private:
    app::Player::Id id_;
    app::Session::Id session_id_;
    model::Dog::Id dog_id_;
};

class PlayerSessionManager {
public:
    PlayerSessionManager() = default;

    explicit PlayerSessionManager(const app::PlayerSessionManager& psm)
        : tokens_(psm.GetAllTokens()) {
        for(const auto& player : psm.GetAllPlayers()) {
            players_.emplace_back(player);
        }
        for(const auto& session : psm.GetAllSessions()) {
            sessions_.emplace_back(session);
        }
    }

    app::PlayerSessionManager Restore() const {


    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {

    }

private:
    app::PlayerSessionManager::TokenToPlayer tokens_;
    std::vector<PlayerRepr> players_;
    std::vector<SessionRepr> sessions_;
};

/* Другие классы модели сериализуются и десериализуются похожим образом */

}  // namespace serialization
