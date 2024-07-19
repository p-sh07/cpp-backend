#pragma once
#include <functional>
#include <optional>

#include "clock.h"
#include "gascooker.h"

/*
Класс "Сосиска".
Позволяет себя обжаривать на газовой плите
*/
class Sausage : public std::enable_shared_from_this<Sausage> {
public:
    using Handler = std::function<void()>;

    explicit Sausage(int id)
        : id_{id} {
    }

    int GetId() const {
        return id_;
    }

    // Асинхронно начинает приготовление. Вызывает handler, как только началось приготовление
    void StartFry(GasCooker& cooker, Handler handler) {
        // Метод StartFry можно вызвать только один раз
        if (frying_start_time_) {
            throw std::logic_error("Frying already started");
        }

        // Запрещаем повторный вызов StartFry
        frying_start_time_ = Clock::now();

        // Готовимся занять газовую плиту
        gas_cooker_lock_ = GasCookerLock{cooker.shared_from_this()};

        // Занимаем горелку для начала обжаривания.
        // Чтобы продлить жизнь текущего объекта, захватываем shared_ptr в лямбде
        cooker.UseBurner([self = shared_from_this(), handler = std::move(handler)] {
            // Запоминаем время фактического начала обжаривания
            self->frying_start_time_ = Clock::now();
            handler();
        });
    }

    // Завершает приготовление и освобождает горелку
    void StopFry() {
        if (!frying_start_time_) {
            throw std::logic_error("Frying has not started");
        }
        if (frying_end_time_) {
            throw std::logic_error("Frying has already stopped");
        }
        frying_end_time_ = Clock::now();
        // Освобождаем горелку
        gas_cooker_lock_.Unlock();
    }

    bool IsCooked() const noexcept {
        return frying_start_time_.has_value() && frying_end_time_.has_value();
    }

    Clock::duration GetCookDuration() const {
        if (!frying_start_time_ || !frying_end_time_) {
            throw std::logic_error("Sausage has not been cooked");
        }
        return *frying_end_time_ - *frying_start_time_;
    }

private:
    int id_;
    GasCookerLock gas_cooker_lock_;
    std::optional<Clock::time_point> frying_start_time_;
    std::optional<Clock::time_point> frying_end_time_;
};

// Класс "Хлеб". Ведёт себя аналогично классу "Сосиска"
class Bread : public std::enable_shared_from_this<Bread> {
public:
    using Handler = std::function<void()>;

    explicit Bread(int id)
        : id_{id} {
    }

    int GetId() const {
        return id_;
    }

    // Начинает приготовление хлеба на газовой плите. Как только горелка будет занята, вызовет
    // handler
    void StartBake(GasCooker& cooker, Handler handler) {
        // Реализуйте этот метод аналогично Sausage::StartFry
        assert(!"Bread::StartBake is not implemented");
        throw std::logic_error("Bread::StartBake is not implemented");
    }

    // Останавливает приготовление хлеба и освобождает горелку.
    void StopBaking() {
        // Реализуйте этот метод по аналогии с Sausage::StopFry
        assert(!"Bread::StopBaking is not implemented");
        throw std::logic_error("Bread::StopBaking is not implemented");
    }

    // Информирует, испечён ли хлеб
    bool IsCooked() const noexcept {
        // Реализуйте этот метод аналогично Sausage::IsCooked
        assert(!"Bread::IsCooked is not implemented");
        return false;
    }

    // Возвращает продолжительность выпекания хлеба. Бросает исключение, если хлеб не был испечён
    Clock::duration GetBakingDuration() const {
        // Реализуйте этот метод аналогично Sausage::GetCookDuration
        assert(!"Bread::GetBakingDuration is not implemented");
        throw std::logic_error("Bread::GetBakingDuration is not implemented");
    }

private:
    int id_;
};

// Склад ингредиентов (возвращает ингредиенты с уникальным id)
class Store {
public:
    std::shared_ptr<Bread> GetBread() {
        return std::make_shared<Bread>(++next_id_);
    }

    std::shared_ptr<Sausage> GetSausage() {
        return std::make_shared<Sausage>(++next_id_);
    }

private:
    int next_id_ = 0;
};
