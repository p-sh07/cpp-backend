#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
using namespace std::literals;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        //async fry and bake, and then make a hot dog
        auto sausage = store_.GetSausage();
        auto bread = store_.GetBread();

        auto strand = net::make_strand(io_);
        //Async fry & bake
        auto sausage_handler = [&]() {
            sausage_cook_timer_.async_wait(
                    net::bind_executor(strand, [self = sausage->shared_from_this()]() {
                        self->IsCooked();
                    }));
        };

        auto bread_handler = [&]() {
            bread_bake_timer_.async_wait(
                    net::bind_executor(strand, [self = bread->shared_from_this()]() {
                        self->IsCooked();
                    }));
        };
        //After both bread and sausage are cooked, make a hotdog
        handler(std::move(HotDog(++next_order_id_, std::move(sausage), std::move(bread))));
    }
private:
    net::io_context& io_;
    int next_order_id_ = 0;

    net::steady_timer sausage_cook_timer_{io_, 1000ms};
    net::steady_timer bread_bake_timer_{io_, 1500ms};

    // Используется для создания ингредиентов хот-дога
    Store store_;

    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);

    // Запускает функцию fn на n потоках, включая текущий
//    template <typename Fn>
//    void RunWorkers(unsigned n, const Fn& fn) {
//        n = std::max(1u, n);
//        std::vector<std::thread> workers;
//        workers.reserve(n - 1);
//        // Запускаем n-1 рабочих потоков, выполняющих функцию fn
//        while (--n) {
//            workers.emplace_back(fn);
//        }
//        fn();
//
//        for (auto& t : workers) {
//            t.join();
//        }
//    }

//    void MakeHotDog(Bread& bread, Sausage& sausage, HotDogHandler& handler) {
//        if(bread.IsCooked() && sausage.IsCooked()) {
//            handler;
//        }
//    }

};

class HotDogManager : public std::enable_shared_from_this<HotDogManager> {
public:
    HotDogManager(net::io_context& io, HotDogHandler handler)
    : io_(io)
    , handler_(std::move(handler))
    , strand_(net::make_strand(io_)) {
    }

    // Запускает асинхронное выполнение заказа
    void Execute(const GasCooker& cooker) {

    }

private:
    net::io_context io_;
    HotDogHandler handler_;
    //Use strand to make sure each hot dog is done without race condition
    net::strand<net::io_context::executor_type> strand_;
    net::steady_timer bread_timer_(io, );



    void RoastSausage() {
        roast_timer_.async_wait(
                as::bind_executor(strand_, [self = shared_from_this()](sys::error_code ec) {
                    self->OnRoasted(ec);
                }));
    }

    void BakeBread() {
        //Pass shared ptr of this class to async_wait via lambda
        marinate_timer_.async_wait(
                as::bind_executor(strand_, [self = shared_from_this()](sys::error_code ec) {
                    self->OnOnionDone(ec);
                }));
    }
};

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




int MakeHamburger(bool with_onion, OrderHandler handler) {
    const int order_id = ++next_order_id_;

    std::make_shared<Order>(io_, order_id, with_onion, std::move(handler))->Execute();

    return order_id;

}

 class Order : public std::enable_shared_from_this<Order> {
    public:
        Order(as::io_context &io, int id, OrderHandler handler)
                : io_(io)
                , id_(id)
                , with_onion_(with_onion)
                , handler_(std::move(handler))
                , strand(as::make_strand(io_)) {
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
