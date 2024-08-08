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

        if (bread_->IsCooked()) {
            DeliverHotDog();
        }
    }

    void OnBreadBaked() {
        if(!bread_->IsCooked()) {
            bread_->StopBaking();
        }
        bread_is_ready_ = true;

        if(sausage_->IsCooked()) {
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
        std::make_shared<OrderManager>(++next_order_id_, io_, *gas_cooker_, store_, std::move(handler))->Execute();
    }
private:
    net::io_context& io_;
    std::atomic_int next_order_id_ = 0;

    // Используется для создания ингредиентов хот-дога
    Store store_;

    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};
