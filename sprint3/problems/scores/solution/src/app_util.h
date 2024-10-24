#pragma once
#include <algorithm>
#include <chrono>
#include <compare>
#include <filesystem>
#include <random>

//DEBUG
#ifdef DEBUG
#include <iostream>
#endif

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

inline bool is_len32hex_num(std::string_view str) {
    return str.size() == 32u
    && std::all_of(str.begin(), str.end(),
                   [](unsigned char c){
        return std::isxdigit(c);
    });
}

namespace fs = std::filesystem;

//Возвращает true, если каталог p содержится внутри base.
inline bool IsSubPath(fs::path path, fs::path base) {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for(auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if(p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

//Конвертирует URL-кодированную строку в путь
inline fs::path ConvertFromUrl(std::string_view url) {
    if(url.empty()) {
        return {};
    }

    std::string path_string;

    for(size_t pos = 0; pos < url.size();) {
        if(url[pos] == '%') {
            //decode
            char decoded = std::stoul(std::string(url.substr(pos + 1, 2)), nullptr, 16);
            path_string.push_back(decoded);
            pos += 3;
        } else {
            path_string.push_back(url[pos++]);
        }
    }
    return path_string;
}

//Конвертирует double Секунды в Миллисекунды chrono
inline std::chrono::milliseconds ConvertSecToMsec(double seconds) {
    return std::chrono::milliseconds(static_cast<long long>(seconds * 1000));
}

}  // namespace util
