#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <cassert>
#include <deque>
#include <memory>

namespace net = boost::asio;
namespace sys = boost::system;

/*
Газовая плита - совместно используемый ресурс кафетерия
Содержит несколько горелок (burner), которые можно асинхронно занимать (метод UseBurner) и
освобождать (метод ReleaseBurner).
Если свободных горелок нет, то запрос на занимание горелки ставится в очередь.
Методы класса можно вызывать из разных потоков.
*/
class GasCooker : public std::enable_shared_from_this<GasCooker> {
public:
    using Handler = std::function<void()>;

    GasCooker(net::io_context& io, int num_burners = 8)
        : io_{io}
        , number_of_burners_{num_burners} {
    }

    GasCooker(const GasCooker&) = delete;
    GasCooker& operator=(const GasCooker&) = delete;

    ~GasCooker() {
        assert(burners_in_use_ == 0);
    }

    // Используется для того, чтобы занять горелку. handler будет вызван в момент, когда горелка
    // занята
    // Этот метод можно вызывать параллельно с вызовом других методов
    void UseBurner(Handler handler) {
        // Выполняем работу внутри strand, чтобы изменение состояния горелки выполнялось
        // последовательно
        net::dispatch(strand_,
                      // За счёт захвата self в лямбда-функции, время жизни GasCooker будет продлено
                      // до её вызова
                      [handler = std::move(handler), self = shared_from_this(), this]() mutable {
                          assert(strand_.running_in_this_thread());
                          assert(burners_in_use_ >= 0 && burners_in_use_ <= number_of_burners_);

                          // Есть свободные горелки?
                          if (burners_in_use_ < number_of_burners_) {
                              // Занимаем горелку
                              ++burners_in_use_;
                              // Асинхронно уведомляем обработчик о том, что горелка занята.
                              // Используется асинхронный вызов, так как handler может
                              // выполняться долго, а strand лучше освободить
                              net::post(io_, std::move(handler));
                          } else {  // Все горелки заняты
                              // Ставим обработчик в хвост очереди
                              pending_handlers_.emplace_back(std::move(handler));
                          }

                          // Проверка инвариантов класса
                          assert(burners_in_use_ > 0 && burners_in_use_ <= number_of_burners_);
                      });
    }

    void ReleaseBurner() {
        // Освобождение выполняем также последовательно
        net::dispatch(strand_, [this, self = shared_from_this()] {
            assert(strand_.running_in_this_thread());
            assert(burners_in_use_ > 0 && burners_in_use_ <= number_of_burners_);

            // Есть ли ожидающие обработчики?
            if (!pending_handlers_.empty()) {
                // Выполняем асинхронно обработчик первый обработчик
                net::post(io_, std::move(pending_handlers_.front()));
                // И удаляем его из очереди ожидания
                pending_handlers_.pop_front();
            } else {
                // Освобождаем горелку
                --burners_in_use_;
            }
        });
    }

private:
    using Strand = net::strand<net::io_context::executor_type>;
    net::io_context& io_;
    Strand strand_{net::make_strand(io_)};
    int number_of_burners_;
    int burners_in_use_ = 0;
    std::deque<Handler> pending_handlers_;
};

// RAII-класс для автоматического освобождения газовой плиты
class GasCookerLock {
public:
    GasCookerLock() = default;

    explicit GasCookerLock(std::shared_ptr<GasCooker> cooker) noexcept
        : cooker_{std::move(cooker)} {
    }

    GasCookerLock(GasCookerLock&& other) = default;
    GasCookerLock& operator=(GasCookerLock&& rhs) = default;

    GasCookerLock(const GasCookerLock&) = delete;
    GasCookerLock& operator=(const GasCookerLock&) = delete;

    ~GasCookerLock() {
        try {
            Unlock();
        } catch (...) {
        }
    }

    void Unlock() {
        if (cooker_) {
            cooker_->ReleaseBurner();
            cooker_.reset();
        }
    }

private:
    std::shared_ptr<GasCooker> cooker_;
};
