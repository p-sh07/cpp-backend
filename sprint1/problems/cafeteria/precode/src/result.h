#pragma once
#include <stdexcept>
#include <type_traits>
#include <variant>

// Вспомогательный класс Result, способный хранить либо значение, либо исключение
template <typename ValueType>
class Result {
public:
    // Конструирует результат, хранящий копию value
    Result(const ValueType& value) noexcept(std::is_nothrow_copy_constructible_v<ValueType>)
        : state_{value} {
    }

    // Конструирует результат, хранящий перемещённое из value значение
    Result(ValueType&& value) noexcept(std::is_nothrow_move_constructible_v<ValueType>)
        : state_{std::move(value)} {
    }

    // Явно запрещаем конструировать из nullptr
    Result(std::nullptr_t) = delete;

    /*
     * Конструирует результат, хранящий исключение.
     * Способ использования:
     *
     * Result<тип> result{std::make_exception_ptr(std::runtime_error{"some error"})};
     */
    Result(std::exception_ptr error)
        : state_{error} {
        if (!error) {
            throw std::invalid_argument("Exception must not be null");
        }
    }

    /*
     * Создаёт результат, который хранит ссылку на текущее выброшенное исключение
     * Способ использования:
     * try {
     *     <код, который может выбросить исключение>
     * } catch (...) {
     *     auto result = Result<тип>::FromCurrentException();
     * }
     */
    static Result FromCurrentException() {
        return std::current_exception();
    }

    // Сообщает, содержится ли внутри значение
    bool HasValue() const noexcept {
        return std::holds_alternative<ValueType>(state_);
    }

    // Если внутри Result хранится исключение, то возвращает указатель на него
    std::exception_ptr GetError() const {
        return std::get<std::exception_ptr>(state_);
    }

    // Если внутри содержится исключение, то выбрасывает его. Иначе не делает ничего.
    void ThrowIfHoldsError() const {
        if (auto e = std::get_if<std::exception_ptr>(&state_)) {
            std::rethrow_exception(*e);
        }
    }

    // Возвращает ссылку на хранящееся значение. Если Result хранит исключение, выбрасывает
    // std::bad_variant_access
    const ValueType& GetValue() const& {
        return std::get<ValueType>(state_);
    }

    // Возвращает rvalue-ссылку на хранящееся значение. Если Result хранит исключение, выбрасывает
    // std::bad_variant_access
    ValueType&& GetValue() && {
        return std::get<ValueType>(std::move(state_));
    }

private:
    std::variant<ValueType, std::exception_ptr> state_;
};
