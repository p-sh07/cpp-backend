#include "sdk.h"

#include <boost/program_options.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

#include "request_handling.h"

namespace net = boost::asio;
namespace sys = boost::system;
namespace json = boost::json;
namespace logging = boost::log;

using namespace std::literals;

struct Args {
    std::string config_path;
    std::string static_root;
    int64_t tick_period = 0;
    bool randomize_spawn_points = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"All options"s};

    Args args;
    desc.add_options()
        // Добавляем опцию --help и её короткую версию -h
        ("help,h", "Show help")
        ("tick-period,t", po::value(&args.tick_period)->value_name("tick period"s), "set tick period")
        ("www-root,w", po::value(&args.static_root)->value_name("static_root"s), "set config file path")
        ("config-file,c", po::value(&args.config_path)->value_name("config_path"s), "set static files root")
        ("randomize_spawn_points", po::bool_switch(&args.randomize_spawn_points), "spawn dogs at random positions");

    // variables_map хранит значения опций после разбора
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        // Если был указан параметр --help, то выводим справку и возвращаем nullopt
        std::cout << desc;
    }

    // Проверяем наличие опций src и dst
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static files path has not been specified"s);
    }
    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file path is not specified"s);
    }

    // С опциями программы всё в порядке, возвращаем структуру args
    return args;
}

namespace {

// Запускает функцию fn на n потоках, включая текущий
template<typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::thread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while(--n) {
        workers.emplace_back(fn);
    }
    fn();

    for(auto& t : workers) {
        t.join();
    }
}

}  // namespace

int main(int argc, const char* argv[]) {
    if(argc < 3) {
        std::cerr << "Usage: game_server <game-config-json> <path-to-static>"sv << std::endl;
        return EXIT_FAILURE;
    }

    //for exit report log
    json::object log_server_exit_report{
        {"code", 0},
        {"exception", ""}
    };

    try {
        // 0. Инициализируем
        server_logger::InitLogging();

        // 0.1. Парсим аргументы переданные в функцию
        auto args = ParseCommandLine(argc, argv);
        if(!args) {
            //failed to parse arguments
            throw std::runtime_error("Failed to parse command line arguments");
        }

        // 1. Загружаем карту из файла, создаем модель и интерфейс (application) игры
        auto game = std::make_shared<model::Game>(json_loader::LoadGame(args->config_path));
        auto game_app = std::make_shared<app::GameInterface>(game);

        // 2. Инициализируем io_context и другие переменные
        game->EnableRandomDogSpawn(args->randomize_spawn_points);
        const auto num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(static_cast<int>(num_threads));
        auto api_strand = net::make_strand(ioc);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if(!ec) {
                ioc.stop();
            }
        });

        //4. Создаем handler и оборачиваем его в логирующий декоратор
        auto handler = std::make_shared<http_handler::RequestHandler>(args->static_root, api_strand, game_app, model::TimeMs{args->tick_period});

        server_logger::LoggingRequestHandler logging_handler{
            [handler](auto&& endpoint, auto&& req, auto&& send) {
                // Обрабатываем запрос
                (*handler)(std::forward<decltype(endpoint)>(endpoint),
                           std::forward<decltype(req)>(req),
                           std::forward<decltype(send)>(send));
            }};

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        // Сервер готов к обработке запросов
        json::object additional_data{{"port", port}, {"address", address.to_string()}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(log_message, "server started")
                                << logging::add_value(log_msg_data, additional_data);


        // 5. Запускаем обработчик HTTP-запросов
        http_server::ServeHttp(ioc, {address, port}, logging_handler);

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch(const std::exception& ex) {
        log_server_exit_report["code"] = EXIT_FAILURE;
        log_server_exit_report["exception"] = ex.what();
    }

    BOOST_LOG_TRIVIAL(info) << logging::add_value(log_message, "server exited")
                            << logging::add_value(log_msg_data, log_server_exit_report);
}
