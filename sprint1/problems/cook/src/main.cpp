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
