
//Example 2, Robot:
#include <boost/asio.hpp>
#include <iostream>

namespace net = boost::asio;
namespace chrono = std::chrono;
using namespace std::literals;
using boost::system::error_code;

// Наследование должно быть публичным, иначе shared_ptr не сможет найти скрытый указатель weak_ptr
class Robot : public std::enable_shared_from_this<Robot> {
public:
    using Duration = net::steady_timer::duration;

    constexpr static double SPEED = 2;  // скорость в м/с
    constexpr static double ROTATION_SPEED = 30;  // скорость поворота (градусов в секунду)

    Robot(net::io_context& io, int id)
        : timer_{io}
        , id_{id} {
    }

    template <typename Callback>
    void Walk(int distance, Callback&& cb) {
        const auto t = 1s * distance / SPEED;
        std::cout << id_ << "> Walk for "sv << t.count() << "sec\n"sv;

        timer_.expires_after(chrono::duration_cast<Duration>(t));

        // self ссылается на текущий объект и продлевает ему время жизни
        timer_.async_wait([distance, cb = std::forward<Callback>(cb),
                           self = shared_from_this()](error_code ec) {
            if (ec) throw std::runtime_error(ec.what());
            self->walk_distance_ += distance;
            std::cout << self->id_ << "> Walked distance: "sv << self->walk_distance_ << "m\n"sv;
            cb();
        });
    }

    template <typename Callback>
    void Rotate(int angle, Callback&& cb) {
        const auto t = 1s * std::abs(angle) / ROTATION_SPEED;
        std::cout << id_ << "> Rotate for "sv << t.count() << "sec\n"sv;

        timer_.expires_after(chrono::duration_cast<Duration>(t));

        timer_.async_wait(
            [angle, cb = std::forward<Callback>(cb), self = shared_from_this()](error_code ec) {
                if (ec) throw std::runtime_error(ec.what());
                self->angle_ = (self->angle_ + angle) % 360;
                std::cout << self->id_ << "> Rotation angle: "sv << self->angle_ << "deg.\n"sv;
                cb();
            });
    }

private:
    net::steady_timer timer_;
    int id_;
    int angle_ = 0;
    int walk_distance_ = 0;
};

void RunRobots(net::io_context& io) {
    auto r1 = std::make_shared<Robot>(io, 1);
    auto r2 = std::make_shared<Robot>(io, 2);

    r1->Rotate(60, [r1] {
        r1->Walk(4, [] {});
    });
    // Внутри лямбда-функции теперь можно захватить даже обычный указатель на текущий объект
    // Он будет валиден до тех пор, пока не завершится вызов функции обработчика
    r2->Walk(2, [r = r2.get()] {
        // Вызов Walk продлит роботу жизнь до завершения асинхронной операции
        r->Walk(3, [] {});
    });
}

int main() {
    net::io_context io;
    RunRobots(io);
    for (;;) {
        try {
            io.run();
            break;
        } catch (const std::exception& e) {
            std::cout << e.what() << '\n';
        }
    }
    std::cout << "Done\n"sv;
}

/* First try:
int main() {
    net::io_context io;

    Robot r1(io, 1);
    Robot r2(io, 2);

    // Робот r1 сперва поворачивается на 60 градусов, а потом идёт 4 метра
    r1.Rotate(60, [&r1] {
        r1.Walk(4, [] {});
    });
    // Робот r2 сперва идёт 2 метра, а потом ещё 3 метра
    r2.Walk(2, [&r2] {
        r2.Walk(3, [] {});
    });

    for (;;) {
        try {
            io.run();
            break;
        } catch (const std::exception& e) {
            std::cout << e.what() << '\n';
        }
    }
    std::cout << "Done\n"sv;
}
*/

//Example 1, Cooking Breakfast:
/*
 *
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>

namespace net = boost::asio;
namespace sys = boost::system;
using namespace std::chrono;
using namespace std::literals;

int main() {
   // Контекст для выполнения операций ввода-вывода.
   // Другие классы библиотеки выполняют с его помощью операции ввода-вывода
   net::io_context io;

   // Замеряем время начала программы
   const auto start_time = steady_clock::now();

   {
       auto t = std::make_shared<net::steady_timer>(io, 3s);
       std::cout << "Fry eggs"sv << std::endl;
       // Захват переменной t внутри лямбда-функции продлит время жизни таймера
       // до тех пор, пока не будет вызван обработчик.
       t->async_wait([t](sys::error_code ec) {
           if (ec) {
               throw std::runtime_error("Wait error: "s + ec.message());
           }
           std::cout << "Put eggs onto the plate. Thread id: "sv << std::this_thread::get_id()
                     << std::endl;
       });
   }

   {
       auto t = std::make_shared<net::steady_timer>(io, 5s);
       std::cout << "Brew coffee"sv << std::endl;
       t->async_wait([t](sys::error_code ec) {
           if (ec) {
               throw std::runtime_error("Wait error: "s + ec.message());
           }
           std::cout << "Pour coffee in the cup. Thread id: "sv << std::this_thread::get_id()
                     << std::endl;
       });
   }

   try {
       std::cout << "Run asynchronous operations"sv << std::endl;
       io.run();
       const auto cook_duration = duration<double>(steady_clock::now() - start_time);
       std::cout << "Breakfast has been cooked in "sv << cook_duration.count() << "s"sv << std::endl;
       std::cout << "Thread id: "sv << std::this_thread::get_id() << std::endl;
       std::cout << "Enjoy your meal"sv << std::endl;
   } catch (const std::exception& e) {
       std::cout << e.what() << std::endl;
   }
}
*/

///NB: Mail widget example code! Useful
/*
class MailWidget : public std::enable_shared_from_this<MailWidget> {
    ...
    // вызывается при нажатии на кнопку "отправить почту"
    void OnSendMailButtonClick() {
        status_label_.SetText("Sending mail"s);

        mailer_.SendMailAsync([weak_self = weak_from_this()](bool success) {
            if (auto self = weak_self.lock()) {
                // Если мы здесь, то MailWidget ещё не разрушен
                // Сильный указатель self продлевает ему жизнь до конца текущего блока

                // Можно безопасно использовать поля класса
                self->status_label_.SetText(
                    success ? "Message has been sent"s : "Message sending error"s);
            }
        });
    }

    // Ссылка на сервис отправки писем
    Mailer& mailer_;
    // Надпись внутри Widget, отображающая его состояние
    Label status_label_;
    ...
};
*/
