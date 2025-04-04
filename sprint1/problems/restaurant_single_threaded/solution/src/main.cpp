#ifdef WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
//#include <syncstream>
#include <unordered_map>
#include <memory>
#include <thread>

namespace as = boost::asio;
namespace sys = boost::system;
namespace ph = std::placeholders;

using namespace std::chrono;

using namespace std::literals;
using Timer = as::steady_timer;

class Hamburger {
public:
    [[nodiscard]] bool IsBurgerRoasted() const {
        return burger_roasted_;
    }
    void SetBurgerRoasted() {
        if (IsBurgerRoasted()) {  // Котлету можно жарить только один раз
            throw std::logic_error("Burger has been roasted already"s);
        }
        burger_roasted_ = true;
    }

    [[nodiscard]] bool HasOnion() const {
        return has_onion_;
    }
    // Добавляем лук
    void AddOnion() {
        if (IsPacked()) {  // Если гамбургер упакован, класть лук в него нельзя
            throw std::logic_error("Hamburger has been packed already"s);
        }
        AssureBurgerRoasted();  // Лук разрешается класть лишь после прожаривания котлеты
        has_onion_ = true;
    }

    [[nodiscard]] bool IsPacked() const {
        return is_packed_;
    }
    void Pack() {
        AssureBurgerRoasted();  // Нельзя упаковывать гамбургер, если котлета не прожарена
        is_packed_ = true;
    }

private:
    // Убеждаемся, что котлета прожарена
    void AssureBurgerRoasted() const {
        if (!burger_roasted_) {
            throw std::logic_error("Bread has not been roasted yet"s);
        }
    }

    bool burger_roasted_ = false;  // Обжарена ли котлета?
    bool has_onion_ = false;       // Есть ли лук?
    bool is_packed_ = false;       // Упакован ли гамбургер?
};

std::ostream& operator<<(std::ostream& os, const Hamburger& h) {
    return os << "Hamburger: "sv << (h.IsBurgerRoasted() ? "roasted burger"sv : " raw burger"sv)
              << (h.HasOnion() ? ", onion"sv : ""sv)
              << (h.IsPacked() ? ", packed"sv : ", not packed"sv);
}

class Logger {
public:
    explicit Logger(std::string id)
        : id_(std::move(id)) {
    }

    void LogMessage(std::string_view message) const {
        //std::osyncstream os{std::cout};
        std::cout << id_ << "> ["sv << duration<double>(steady_clock::now() - start_time_).count()
           << "s] "sv << message << std::endl;
    }

private:
    std::string id_;
    steady_clock::time_point start_time_{steady_clock::now()};
};

// Функция, которая будет вызвана по окончании обработки заказа
using OrderHandler = std::function<void(sys::error_code ec, int id, Hamburger* hamburger)>;

class Order : public std::enable_shared_from_this<Order> {
public:
    Order(as::io_context& io, int id, bool with_onion, OrderHandler handler)
            : io_(io)
            , id_(id)
            , with_onion_(with_onion)
            , handler_(std::move(handler)) {
    }

    // Запускает асинхронное выполнение заказа
    void Execute() {
        logger_.LogMessage("Order has been started."sv);
        RoastBurger();
        if(with_onion_) {
            MarinateOnion();
        }
    }
private:
    as::io_context& io_;
    int id_;

    bool with_onion_ = false, onion_done_ = false,
    delivered_ = false;

    OrderHandler handler_;

    Hamburger hamburger_;

    Logger logger_{std::to_string(id_)};
    Timer roast_timer_{io_, 1s};
    Timer marinate_timer_{io_, 2s};;


    void RoastBurger() {
        logger_.LogMessage("Start roasting burger... [Async timer -> start {1 sec}]"sv);
        roast_timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
            self->OnRoasted(ec);
        });
    }

    void MarinateOnion() {
        logger_.LogMessage("Start marinating onion... [Async timer -> start {2 sec}]"sv);

        //pass shared ptr of this class to async_wait via lambda
        marinate_timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
            self->OnOnionDone(ec);
        });
    }

    void OnRoasted(sys::error_code ec) {
        if (ec) {
            logger_.LogMessage("Roast error : "s + ec.what());
        } else {
            logger_.LogMessage("Burger roasted! [Async Handler -> done]"sv);
            hamburger_.SetBurgerRoasted();
        }
        CheckReadiness(ec);
    }

    void OnOnionDone(sys::error_code ec) {
        if (ec) {
            logger_.LogMessage("Marinate onion error: "s + ec.what());
        } else {
            logger_.LogMessage("Onion done! [Async Handler  -> done]"sv);
            onion_done_ = true;
        }
        CheckReadiness(ec);
    }

    void CheckReadiness(sys::error_code ec) {
        if (delivered_) {
            logger_.LogMessage("Hamburger Delivered! *end*");
            // Выходим, если заказ уже доставлен либо клиента уведомили об ошибке
            return;
        }
        if (ec) {
            logger_.LogMessage("Hamburger Failed! *sent error*");
            // В случае ошибки уведомляем клиента о невозможности выполнить заказ
            return Deliver(ec);
        }

        // Самое время добавить лук
        if (CanAddOnion()) {
            logger_.LogMessage("Added onion"sv);
            hamburger_.AddOnion();
        }

        // Если все компоненты гамбургера готовы, упаковываем его
        if (IsReadyToPack()) {
            Pack();
        }
    }

    void Deliver(sys::error_code ec) {
        // Защита заказа от повторной доставки
        delivered_ = true;
        // Доставляем гамбургер в случае успеха либо nullptr, если возникла ошибка
        handler_(ec, id_, ec ? nullptr : &hamburger_);
    }

    [[nodiscard]] bool CanAddOnion() const {
        // Лук можно добавить, если котлета обжарена, лук замаринован, но пока не добавлен
        return hamburger_.IsBurgerRoasted() && onion_done_ && !hamburger_.HasOnion();
    }

    [[nodiscard]] bool IsReadyToPack() const {
        // Если котлета обжарена и лук добавлен, как просили, гамбургер можно упаковывать
        return hamburger_.IsBurgerRoasted() && (!with_onion_ || hamburger_.HasOnion());
    }

    void Pack() {
        logger_.LogMessage("Packing...[Sync Wait -> start {0.5 sec}]"sv);

        // Просто потребляем ресурсы процессора в течение 0,5 с.
        auto start = steady_clock::now();
        while (steady_clock::now() - start < 500ms) {
        }

        hamburger_.Pack();
        logger_.LogMessage("Packed"sv);

        Deliver({});
    }
};

class Restaurant {
public:
    explicit Restaurant(as::io_context& io)
        : io_(io) {
    }

    int MakeHamburger(bool with_onion, OrderHandler handler) {
        const int order_id = ++next_order_id_;

        std::make_shared<Order>(io_, order_id, with_onion, std::move(handler))->Execute();

        return order_id;
    }

private:
    as::io_context& io_;
    int next_order_id_ = 0;
};

int main() {
    as::io_context io;

    Restaurant restaurant{io};

    Logger logger{"main"s};

    struct OrderResult {
        sys::error_code ec;
        Hamburger hamburger;
    };

    std::unordered_map<int, OrderResult> orders;
    auto handle_result = [&](sys::error_code ec, int id, Hamburger* h) {
        orders.emplace(id, OrderResult{ec, ec ? Hamburger{} : *h});
        logger.LogMessage("Order #"s + std::to_string(id) + " Delivered! [Async Handler -> done]"s);
    };

    const int id1 = restaurant.MakeHamburger(false, handle_result);
    const int id2 = restaurant.MakeHamburger(true, handle_result);
    // const int id3 = restaurant.MakeHamburger(false, handle_result);
    // const int id4 = restaurant.MakeHamburger(true, handle_result);

    std::vector<std::thread> workers;

    // До вызова io.run() никакие заказы не выполняются
    assert(orders.empty());

    //Launch threads - simple approach
    // for(int i = 0; i < 1; ++i) {
    //     // auto io_run = [&io]() {
    //     //     io.run();
    //     // };

    //     workers.emplace_back(boost::bind(&as::io_context::run, &io));

    //     std::stringstream ss;
    //     ss << workers.back().get_id();

    //     std::cout << "->Thread [" + ss.str() + "] started" << std::endl;

    //     for(auto& t : workers) {
    //         t.join();
    //     }
    // }

    //Run Async ops.
    io.run();

    // После вызова io.run() все заказы быть выполнены
    assert(orders.size() == 2u);
    {
        // Проверяем заказ без лука
        const auto& o = orders.at(id1);
        assert(!o.ec);
        assert(o.hamburger.IsBurgerRoasted());
        assert(o.hamburger.IsPacked());
        assert(!o.hamburger.HasOnion());
    }
    {
        // Проверяем заказ с луком
        const auto& o = orders.at(id2);
        assert(!o.ec);
        assert(o.hamburger.IsBurgerRoasted());
        assert(o.hamburger.IsPacked());
        assert(o.hamburger.HasOnion());
    }
}
