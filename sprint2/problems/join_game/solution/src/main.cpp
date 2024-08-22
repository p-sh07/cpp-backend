#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

#include "boost_log.h"
#include "json_loader.h"
#include "player_manager.h"
#include "request_handler.h"

namespace net = boost::asio;
namespace sys = boost::system;
namespace json = boost::json;
namespace logging = boost::log;

using namespace std::literals;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::thread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();

    for(auto& t : workers) {
        t.join();
    }
}

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <path-to-static>"sv << std::endl;
        return EXIT_FAILURE;
    }

    //for exit report log
    json::object log_server_exit_report {
            {"code", 0},
            {"exception", ""}
    };

    try {
        // 0. Инициализируем
        bstlog::InitLogging();

        // 1. Загружаем карту из файла и создаем модель игры
        auto game_instance = std::make_shared<model::Game>(json_loader::LoadGame(argv[1]));
        auto player_manager = std::make_shared<game::PlayerManager>(game_instance);

        // 2. Инициализируем логгер, io_context и другие переменные
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        auto api_strand = net::make_strand(ioc);

        auto handler = std::make_shared<http_handler::RequestHandler>(
            static_files_root, api_strand, game_manager
        );


        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler handler{game, argv[2]};

        // 5. Запускаем обработчик HTTP-запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

// Оборачиваем его в логирующий декоратор
server_logging::LoggingRequestHandler logging_handler{
    [handler](auto&& endpoint, auto&& req, auto&& send) {
        // Обрабатываем запрос
        (*handler)(std::forward<decltype(endpoint)>(endpoint),
            std::forward<decltype(req)>(req),
            std::forward<decltype(send)>(send));
            }};

        // Сервер готов к обработке запросов
        json::object additional_data{{"port", port},{"address", address.to_string()}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(log_message, "server started")
                                << logging::add_value(log_msg_data, additional_data);

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        log_server_exit_report["code"] = EXIT_FAILURE;
        log_server_exit_report["exception"] = ex.what();
    }

    BOOST_LOG_TRIVIAL(info) << logging::add_value(log_message, "server exited")
                            << logging::add_value(log_msg_data, log_server_exit_report);
}
