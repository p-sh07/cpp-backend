#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/bind.hpp>
#include <memory>
#include <thread>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
using namespace std::literals;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

//Класс для обработки заказа
class OrderManager : public std::enable_shared_from_this<OrderManager> {
public:
    OrderManager(int order_id, net::io_context& io, GasCooker& cooker, Store& store, HotDogHandler handler)
        : id_(order_id)
        , io_(io)
        , strand_(net::make_strand(io_))
        , cooker_(cooker)
        , handler_(std::move(handler))
        , sausage_(store.GetSausage())
        , bread_(store.GetBread()) {
    }

    // Запускает асинхронное выполнение заказа
    void Execute() {
        sausage_->StartFry(cooker_, [self = shared_from_this()] {
            self->RoastSausage();
        });

        bread_->StartBake(cooker_, [self = shared_from_this()]() {
            self->BakeBread();
        });
        //->sausage and bread handlers will call DeliverHotDog() if the other ingredient is ready
    }

    int GetId() {
        return id_;
    }

    const Sausage& GetSausage() const noexcept {
        return *sausage_;
    }

    const Bread& GetBread() const noexcept {
        return *bread_;
    }


private:
    int id_;
    net::io_context& io_;
    net::strand<net::io_context::executor_type> strand_;
    GasCooker& cooker_;

    HotDogHandler handler_;

    std::shared_ptr<Sausage> sausage_ = nullptr;
    std::shared_ptr<Bread> bread_ = nullptr;

    bool sausage_is_ready_ = false;
    bool bread_is_ready_ = false;

    void OnSausageCooked() {
        if(!sausage_->IsCooked()) {
            sausage_->StopFry();
        }
        sausage_is_ready_ = true;

        if (bread_is_ready_) {
            DeliverHotDog();

            // Could also just use this? - post or dispatch, no difference here
            // net::post(strand_,[self = shared_from_this()]() {
            //     self->MakeHotDog();
            // });
        }
    }

    void OnBreadBaked() {
        if(!bread_->IsCooked()) {
            bread_->StopBaking();
        }
        bread_is_ready_ = true;

        if(sausage_is_ready_) {
            DeliverHotDog();
        }
    }

    void RoastSausage() {
        auto timer = std::make_shared<net::steady_timer>(io_, HotDog::MIN_SAUSAGE_COOK_DURATION);
        timer->async_wait(
            net::bind_executor(strand_, [self = shared_from_this(), timer](sys::error_code ec) {
                self->OnSausageCooked();
            }));
    }

    void BakeBread() {
        auto timer = std::make_shared<net::steady_timer>(io_, HotDog::MIN_BREAD_COOK_DURATION);
        timer->async_wait(
            net::bind_executor(strand_, [self = shared_from_this(), timer](sys::error_code ec) {
                self->OnBreadBaked();
            }));
    }

    Result<HotDog> MakeHotDog() {

        if(!sausage_is_ready_ || !bread_is_ready_) {
            throw std::logic_error("Ingredients are not ready yet, cannot Make");
        }

        try {
            std::cout << id_ << "> Attempting to make hot dog..." << std::endl;
            return {HotDog(id_, sausage_->shared_from_this(), bread_->shared_from_this())};
        } catch (...) {
            std::cout << id_ << "> MakeDog Error occured!" << std::endl;
            return Result<HotDog>::FromCurrentException();
        }
    }

    void DeliverHotDog() {
        net::dispatch(strand_, [self = shared_from_this()] () {
            self->handler_(self->MakeHotDog());
        });
    }
};

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        std::make_shared<OrderManager>(order_id, io_, *gas_cooker_, store_, std::move(handler))->Execute();
    }
private:
    net::io_context& io_;
    int next_order_id_ = 0;

    // Используется для создания ингредиентов хот-дога
    Store store_;

    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
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

// Обработчик заказа может быть вызван в любом из потоков, вызывающих io.run().
// Чтобы избежать состояния гонки при обращении к orders, выполняем обращения к orders через
// strand, используя функцию dispatch.
//auto handle_result
//        = [strand = as::make_strand(io), &orders](sys::error_code ec, int id, Hamburger* h) {
//            as::dispatch(strand, [&orders, id, res = OrderResult{ec, ec ? Hamburger{} : *h}] {
//                orders.emplace(id, res);
//            });
//        };
//
//const int num_orders = 16;
//for (int i = 0; i < num_orders; ++i) {
//restaurant.MakeHamburger(i % 2 == 0, handle_result);
//}




//int MakeHamburger(bool with_onion, OrderHandler handler) {
//    const int order_id = ++next_order_id_;
//
//    std::make_shared<Order>(io_, order_id, with_onion, std::move(handler))->Execute();
//
//    return order_id;
//
//}
//
// class Order : public std::enable_shared_from_this<Order> {
//    public:
//        Order(as::io_context &io, int id, OrderHandler handler)
//                : io_(io)
//                , id_(id)
//                , with_onion_(with_onion)
//                , handler_(std::move(handler))
//                , strand(as::make_strand(io_)) {
//        }
//
//    private:
//        as::io_context &io_;
//        //std::atomic_int counter_ = 0;
//
//        //One way to prevent race
//        //std::mutex mutex_;
//
//        int id_;
//
//        bool with_onion_ = false;
//        bool onion_done_ = false;
//        bool delivered_ = false;
//
//        OrderHandler handler_;
//
//        Hamburger hamburger_;
//
//        Logger logger_{std::to_string(id_)};
//        Timer roast_timer_{io_, 1s};
//        Timer marinate_timer_{io_, 2s};
//
//        net::strand<net::io_context::executor_type> strand_;
//
//        void RoastBurger() {
//            logger_.LogMessage("Start roasting burger..."sv);
//            roast_timer_.async_wait(
//                    as::bind_executor(strand_, [self = shared_from_this()](sys::error_code ec) {
//                        self->OnRoasted(ec);
//                    }));
//        }
//
//        void MarinateOnion() {
//            logger_.LogMessage("Start marinating onion..."sv);
//
//            //Pass shared ptr of this class to async_wait via lambda
//            marinate_timer_.async_wait(
//                    as::bind_executor(strand_, [self = shared_from_this()](sys::error_code ec) {
//                        self->OnOnionDone(ec);
//                    }));
//        }
//
//        void OnRoasted(sys::error_code ec) {
//            //A way to check for RaceCondition
//            //ThreadChecker checker{counter_};
//
//            //To prevent parallel mods
//            //std::lock_guard lk{mutex_};
//
//            if (ec) {
//                logger_.LogMessage("Roast error : "s + ec.what());
//            } else {
//                logger_.LogMessage("Burger has been roasted."sv);
//                hamburger_.SetBurgerRoasted();
//            }
//            CheckReadiness(ec);
//        }
//
//        void OnOnionDone(sys::error_code ec) {
//            //A way to check for RaceCondition
//            //ThreadChecker checker{counter_};
//
//            //To prevent parallel mods
//            //std::lock_guard lk{mutex_};
//
//            logger_.LogMessage("On Onion done"sv);
//            if (ec) {
//                logger_.LogMessage("Marinate onion error: "s + ec.what());
//            } else {
//                logger_.LogMessage("Onion has been marinated."sv);
//                onion_done_ = true;
//            }
//            CheckReadiness(ec);
//        }
//
//        void CheckReadiness(sys::error_code ec) {
//            if (delivered_) {
//                logger_.LogMessage("Hamburger Delivered! *end*");
//                // Выходим, если заказ уже доставлен либо клиента уведомили об ошибке
//                return;
//            }
//            if (ec) {
//                logger_.LogMessage("Hamburger Failed! *sent error*");
//                // В случае ошибки уведомляем клиента о невозможности выполнить заказ
//                return Deliver(ec);
//            }
//
//            // Самое время добавить лук
//            if (CanAddOnion()) {
//                logger_.LogMessage("Adding onion..."sv);
//                hamburger_.AddOnion();
//            }
//
//            // Если все компоненты гамбургера готовы, упаковываем его
//            if (IsReadyToPack()) {
//                Pack();
//            }
//        }
//
//        void Deliver(sys::error_code ec) {
//            // Защита заказа от повторной доставки
//            delivered_ = true;
//            // Доставляем гамбургер в случае успеха либо nullptr, если возникла ошибка
//            handler_(ec, id_, ec ? nullptr : &hamburger_);
//        }
//
//        [[nodiscard]] bool CanAddOnion() const {
//            // Лук можно добавить, если котлета обжарена, лук замаринован, но пока не добавлен
//            return hamburger_.IsBurgerRoasted() && onion_done_ && !hamburger_.HasOnion();
//        }
//
//        [[nodiscard]] bool IsReadyToPack() const {
//            // Если котлета обжарена и лук добавлен, как просили, гамбургер можно упаковывать
//            return hamburger_.IsBurgerRoasted() && (!with_onion_ || hamburger_.HasOnion());
//        }
//
//        void Pack() {
//            logger_.LogMessage("Packing..."sv);
//
//            // Просто потребляем ресурсы процессора в течение 0,5 с.
//            auto start = steady_clock::now();
//            while (steady_clock::now() - start < 500ms) {
//            }
//
//            hamburger_.Pack();
//            logger_.LogMessage("Packed"sv);
//
//            Deliver({});
//        }
//    };
