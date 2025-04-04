#pragma once
#include <compare>
#include <random>

//DEBUG
#include "iostream"

namespace util {

/**
 * Вспомогательный шаблонный класс "Маркированный тип".
 * С его помощью можно описать строгий тип на основе другого типа.
 * Пример:
 *
 *  struct AddressTag{}; // метка типа для строки, хранящей адрес
 *  using Address = util::Tagged<std::string, AddressTag>;
 *
 *  struct NameTag{}; // метка типа для строки, хранящей имя
 *  using Name = util::Tagged<std::string, NameTag>;
 *
 *  struct Person {
 *      Name name;
 *      Address address;
 *  };
 *
 *  Name name{"Harry Potter"s};
 *  Address address{"4 Privet Drive, Little Whinging, Surrey, England"s};
 *
 * Person p1{name, address}; // OK
 * Person p2{address, name}; // Ошибка, Address и Name - разные типы
 */
template<typename Value, typename Tag>
class Tagged {
 public:
    using ValueType = Value;
    using TagType = Tag;

    explicit Tagged(Value&& v)
        : value_(std::move(v)) {
    }
    explicit Tagged(const Value& v)
        : value_(v) {
    }

    const Value& operator*() const {
        return value_;
    }

    Value& operator*() {
        return value_;
    }

    Value& GetVal() {
        return value_;
    }

    const Value& GetVal() const {
        return value_;
    }

    // Так в C++20 можно объявить оператор сравнения Tagged-типов
    // Будет просто вызван соответствующий оператор для поля value_
    auto operator<=>(const Tagged<Value, Tag>&) const = default;

 private:
    Value value_;
};

// Хешер для Tagged-типа, чтобы Tagged-объекты можно было хранить в unordered-контейнерах
template<typename TaggedValue>
struct TaggedHasher {
    size_t operator()(const TaggedValue& value) const {
        // Возвращает хеш значения, хранящегося внутри value
        return std::hash<typename TaggedValue::ValueType>{}(*value);
    }
};

inline size_t random_num(size_t limit1, size_t limit2) {
    thread_local static std::mt19937 rg{std::random_device{}()};

    //in case when min max are swapped:
    size_t min = std::min(limit1, limit2);
    size_t max = std::max(limit1, limit2);

    thread_local static std::uniform_int_distribution<size_t> dist(min, max);
    return dist(rg);
}

inline bool is_len32hex_num(std::string_view str) {
    return str.size() == 32u
    && std::all_of(str.begin(), str.end(),
                   [](unsigned char c){
        return std::isxdigit(c);
    });
}

}  // namespace util
