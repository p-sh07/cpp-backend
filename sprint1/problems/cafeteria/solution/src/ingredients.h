#pragma once
#include <functional>
#include <optional>

#include <iostream>
//#include <syncstream>

#include "clock.h"
#include "gascooker.h"

using namespace std::literals;
namespace sc = std::chrono;

// class DebugLog {
// public:
//     explicit DebugLog(std::string id)
//         : id_(std::move(id)) {
//     }

//     void LogMessage(std::string_view message) const {
//         std::osyncstream os{std::cout};
//         os << message << std::endl;
//     }

// private:
//     std::string id_;
// };

/*
Класс "Сосиска".
Позволяет себя обжаривать на газовой плите
*/
class Sausage : public std::enable_shared_from_this<Sausage> {
public:

    using Handler = std::function<void()>;

    explicit Sausage(int id)
        : id_{id}
    //    , log_(std::to_string(id))
    {
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
        // log_.LogMessage("->stopped frying sausage at: " + std::to_string(sc::duration_cast<sc::milliseconds>(*frying_end_time_-*frying_start_time_).count()));
       
        // Освобождаем горелку
        gas_cooker_lock_.Unlock();
    }

    bool IsCooked() const noexcept {
        return frying_start_time_.has_value() && frying_end_time_.has_value();
    }

    Clock::duration GetCookDuration() const {
        if (!frying_start_time_.has_value()) {
            throw std::logic_error("Bread has not started been baked");
        } else if(!frying_end_time_) {
            return Clock::now() - *frying_start_time_;
        }
        return *frying_end_time_ - *frying_start_time_;
    }

private:
    int id_;
    GasCookerLock gas_cooker_lock_;
    // DebugLog log_;
    std::optional<Clock::time_point> frying_start_time_;
    std::optional<Clock::time_point> frying_end_time_;
};

// Класс "Хлеб". Ведёт себя аналогично классу "Сосиска"
class Bread : public std::enable_shared_from_this<Bread> {
public:
    using Handler = std::function<void()>;

    explicit Bread(int id)
        : id_{id}
    //    , log_(std::to_string(id))
    {
    }

    int GetId() const {
        return id_;
    }

    // Начинает приготовление хлеба на газовой плите. Как только горелка будет занята, вызовет
    // handler
    void StartBake(GasCooker& cooker, Handler handler) {
        // Реализуйте этот метод аналогично Sausage::StartFry
        if (baking_start_time_) {
            throw std::logic_error("Baking already started");
        }

        // Запрещаем повторный вызов StartBake
        baking_start_time_ = Clock::now();

        // Готовимся занять газовую плиту
        gas_cooker_lock_ = GasCookerLock{cooker.shared_from_this()};

        // Занимаем горелку для начала обжаривания.
        // Чтобы продлить жизнь текущего объекта, захватываем shared_ptr в лямбде
        cooker.UseBurner([self = shared_from_this(), handler = std::move(handler)] {
            // Запоминаем время фактического начала обжаривания
            self->baking_start_time_ = Clock::now();
            handler();
        });
    }

    // Останавливает приготовление хлеба и освобождает горелку.
    void StopBaking() {

        if (!baking_start_time_) {
            throw std::logic_error("Baking has not started");
        }
        if (baking_end_time_) {
            throw std::logic_error("Baking has already stopped");
        }
        baking_end_time_ = Clock::now();
        // log_.LogMessage("->stopped baking bread at: " + std::to_string(sc::duration_cast<sc::milliseconds>(*baking_end_time_-*baking_start_time_).count()));
     
        // Освобождаем горелку
        gas_cooker_lock_.Unlock();
    }

    // Информирует, испечён ли хлеб
    bool IsCooked() const noexcept {
        return baking_start_time_.has_value() && baking_end_time_.has_value();
    }

    // Возвращает продолжительность выпекания хлеба. Бросает исключение, если хлеб не был испечён
    Clock::duration GetBakingDuration() const {
        if (!baking_start_time_) {
            //throw std::logic_error("Bread has not been baked");
        } else if(!baking_end_time_) {
            return Clock::now() - *baking_start_time_;
        }

        return *baking_end_time_ - *baking_start_time_;
    }

private:
    int id_;
    GasCookerLock gas_cooker_lock_;
    // DebugLog log_;
    std::optional<Clock::time_point> baking_start_time_;
    std::optional<Clock::time_point> baking_end_time_;
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
