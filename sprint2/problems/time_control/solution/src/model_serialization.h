#include <boost/serialization/vector.hpp>

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
void serialize(Archive& ar, LootItemInfo& obj, [[maybe_unused]] const unsigned version) {
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
        , name_(*dog.GetTag())
        , pos_(dog.GetPos())
        , bag_capacity_(dog.GetBagCap())
        , speed_(dog.GetSpeed())
        , direction_(dog.GetDirection())
        , score_(dog.GetScore())
        , bag_content_(dog.GetBag()) {
    }

    [[nodiscard]] model::Dog Restore() const {
        model::Dog dog{id_, pos_, width_, name_, bag_capacity_};
        dog.SetSpeed(speed_);
        dog.SetDirection(direction_);
        dog.AddScore(score_);
        for (const auto& item : bag_content_) {
            if (!dog.CollectItem(item)) {
                throw std::runtime_error("Failed to put bag content");
            }
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& name_;
        ar& pos_;
        ar& width_;
        ar& bag_capacity_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
    }

private:
    model::Dog::Id id_ = model::Dog::Id{0u};
    model::Dog::Tag name_ = model::Dog::Tag{""};
    model::Point2D pos_;
    double width_;
    size_t bag_capacity_ = 0;
    model::Speed speed_;
    model::Direction direction_ = model::Direction::NORTH;
    model::Score score_ = 0;
    model::Dog::BagContent bag_content_;
};

/* Другие классы модели сериализуются и десериализуются похожим образом */

}  // namespace serialization
