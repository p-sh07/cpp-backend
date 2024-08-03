#ifdef WIN32
 #include <sdkddkver.h>
 #endif

#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <syncstream>
#include <thread>

namespace as = boost::asio;
namespace sys = boost::system;
namespace ph = std::placeholders;

using namespace std::chrono;
using namespace std::literals;

using Timer = as::steady_timer;

namespace {

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
    return os << "Hamburger: "sv << (h.IsBurgerRoasted() ? "roasted cutlet"sv : " raw cutlet"sv)
              << (h.HasOnion() ? ", onion"sv : ""sv)
              << (h.IsPacked() ? ", packed"sv : ", not packed"sv);
}

class Logger {
public:
    explicit Logger(std::string id)
            : id_(std::move(id)) {
    }

    void LogMessage(std::string_view message) const {
        std::osyncstream os{std::cout};
        os << id_ << "> ["sv << duration<double>(steady_clock::now() - start_time_).count()
                  << "s] "sv << message << std::endl;
    }

private:
    std::string id_;
    steady_clock::time_point start_time_{steady_clock::now()};
};

class ThreadChecker {
public:
    explicit ThreadChecker(std::atomic_int& counter)
        : counter_{counter} {
    }

    ThreadChecker(const ThreadChecker&) = delete;
    ThreadChecker& operator=(const ThreadChecker&) = delete;

    ~ThreadChecker() {
        // assert выстрелит, если между вызовом конструктора и деструктора
        // значение expected_counter_ изменится
        assert(expected_counter_ == counter_);
    }

private:
    std::atomic_int& counter_;
    int expected_counter_ = ++counter_;
};

// Функция, которая будет вызвана по окончании обработки заказа
using OrderHandler = std::function<void(sys::error_code ec, int id, Hamburger* hamburger)>;

class Order : public std::enable_shared_from_this<Order> {
public:
    Order(as::io_context &io, int id, bool with_onion, OrderHandler handler)
        : io_(io)
        , id_(id)
        , with_onion_(with_onion)
        , handler_(std::move(handler))
        , strand(as::make_strand(io_)) {
    }

    // Запускает асинхронное выполнение заказа
    void Execute() {
        logger_.LogMessage("Order has been started."sv);
        RoastBurger();
        if (with_onion_) {
            MarinateOnion();
        }
    }

private:
    as::io_context &io_;
    //std::atomic_int counter_ = 0;
    
    //One way to prevent race
    //std::mutex mutex_;
    
    int id_;

    bool with_onion_ = false;
    bool onion_done_ = false;
    bool delivered_ = false;

    OrderHandler handler_;

    Hamburger hamburger_;

    Logger logger_{std::to_string(id_)};
    Timer roast_timer_{io_, 1s};
    Timer marinate_timer_{io_, 2s};

    net::strand<net::io_context::executor_type> strand_;

    void RoastBurger() {
        logger_.LogMessage("Start roasting burger..."sv);
        roast_timer_.async_wait(
            as::bind_executor(strand_, [self = shared_from_this()](sys::error_code ec) {
                self->OnRoasted(ec);
        }));
    }

    void MarinateOnion() {
        logger_.LogMessage("Start marinating onion..."sv);

        //Pass shared ptr of this class to async_wait via lambda
        marinate_timer_.async_wait(
            as::bind_executor(strand_, [self = shared_from_this()](sys::error_code ec) {
            self->OnOnionDone(ec);
        }));
    }

    void OnRoasted(sys::error_code ec) {
        //A way to check for RaceCondition
        //ThreadChecker checker{counter_};

        //To prevent parallel mods
        //std::lock_guard lk{mutex_};

        if (ec) {
            logger_.LogMessage("Roast error : "s + ec.what());
        } else {
            logger_.LogMessage("Burger has been roasted."sv);
            hamburger_.SetBurgerRoasted();
        }
        CheckReadiness(ec);
    }

    void OnOnionDone(sys::error_code ec) {
        //A way to check for RaceCondition
        //ThreadChecker checker{counter_};

        //To prevent parallel mods
        //std::lock_guard lk{mutex_};

        logger_.LogMessage("On Onion done"sv);
        if (ec) {
            logger_.LogMessage("Marinate onion error: "s + ec.what());
        } else {
            logger_.LogMessage("Onion has been marinated."sv);
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
            logger_.LogMessage("Adding onion..."sv);
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
        logger_.LogMessage("Packing..."sv);

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



// Запускает функцию fn на n потоках, включая текущий
    template <typename Fn>
    void RunWorkers(unsigned n, const Fn& fn) {
        n = std::max(1u, n);
        std::vector<std::jthread> workers;
        workers.reserve(n - 1);
        // Запускаем n-1 рабочих потоков, выполняющих функцию fn
        while (--n) {
            workers.emplace_back(fn);
        }
        fn();
    }

}  // namespace

int main() {
    const unsigned num_workers = 4;
    as::io_context io(num_workers);

    Restaurant restaurant{io};

    Logger logger{"main"s};

    struct OrderResult {
        sys::error_code ec;
        Hamburger hamburger;
    };

    std::unordered_map<int, OrderResult> orders;

    // Обработчик заказа может быть вызван в любом из потоков, вызывающих io.run().
    // Чтобы избежать состояния гонки при обращении к orders, выполняем обращения к orders через
    // strand, используя функцию dispatch.
    auto handle_result
            = [strand = as::make_strand(io), &orders](sys::error_code ec, int id, Hamburger* h) {
                as::dispatch(strand, [&orders, id, res = OrderResult{ec, ec ? Hamburger{} : *h}] {
                    orders.emplace(id, res);
                });
            };

    const int num_orders = 16;
    for (int i = 0; i < num_orders; ++i) {
        restaurant.MakeHamburger(i % 2 == 0, handle_result);
    }

    assert(orders.empty());
    RunWorkers(num_workers, [&io] {
        io.run();
    });
    assert(orders.size() == num_orders);

    for (const auto& [id, order] : orders) {
        assert(!order.ec);
        assert(order.hamburger.IsBurgerRoasted());
        assert(order.hamburger.IsPacked());
        assert(order.hamburger.HasOnion() == (id % 2 != 0));
    }
}
