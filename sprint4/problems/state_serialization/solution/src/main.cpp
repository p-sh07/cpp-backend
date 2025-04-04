#include "sdk.h"

#include <boost/program_options.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>
#include <boost/serialization/serialization.hpp>

#include "request_handling.h"
#include "state_serialization.h"

namespace net = boost::asio;
namespace sys = boost::system;
namespace json = boost::json;
namespace logging = boost::log;

using namespace std::literals;

struct Args {
    //Required params
    std::string config_path;
    std::string static_root;

    //Optional params
    int64_t tick_period         = 0;
    bool randomize_spawn_points = false;
    std::string state_file      = "";
    bool enable_save            = false;
    int64_t save_period         = 0;
    bool enable_periodic_save   = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"All options"s};

    Args args;
    desc.add_options()
        // Добавляем опцию --help и её короткую версию -h
        ("help,h", "Show help")
        ("tick-period,t", po::value(&args.tick_period)->value_name("tick_period"s), "set tick period")
        ("www-root,w", po::value(&args.static_root)->value_name("static_root"s), "set config file path")
        ("config-file,c", po::value(&args.config_path)->value_name("config_path"s), "set static files root")
        ("randomize_spawn_points", po::bool_switch(&args.randomize_spawn_points), "spawn dogs at random positions")
        ("state-file,f", po::value(&args.state_file)->value_name("state_file"s), "set save file path")
        ("save-state-period,p", po::value(&args.save_period)->value_name("save_period"s), "set state save interval");

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

    // Explicitly set bool if option is given
    args.enable_save          = vm.contains("state-file"s);
    args.enable_periodic_save = vm.contains("save-state-period"s);

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
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();

    for (auto& t : workers) {
        t.join();
    }
}
} // namespace

int main(int argc, const char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: game_server <game-config-json> <path-to-static>"sv << std::endl;
        return EXIT_FAILURE;
    }

    //for exit report log
    json::object log_server_exit_report{
        {"code", 0}, {"exception", ""}
    };


    try {
        // 0. Инициализируем
        server_logger::InitLogging();

        // 0.1. Парсим аргументы переданные в функцию
        auto args = ParseCommandLine(argc, argv);
        if (!args) {
            //failed to parse arguments
            throw std::runtime_error("Failed to parse command line arguments");
        }

        // 1. Инициализируем io_context и другие переменные
        const auto num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(static_cast<int>(num_threads));
        auto api_strand          = net::make_strand(ioc);

        auto serializer_listener = args->enable_save
            ? std::make_shared<serialization::StateSerializer>(args->state_file, args->enable_periodic_save, args->save_period)
            : nullptr;

        // 2. Загружаем карту из файла, создаем модель и интерфейс (application) игры
        auto game = std::make_shared<model::Game>(json_loader::LoadGame(args->config_path));
        game->EnableRandomDogSpawn(args->randomize_spawn_points);

        // 2.1. При наличии сохраненного состояния, восстанавливаем данные из файла //TODO: Restore throw if unsuccessful
        auto game_app = std::make_shared<app::GameInterface>(ioc, game, serializer_listener);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc, game_app, serializer_listener](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                //TODO: does this work?
                if (serializer_listener) {
                    serializer_listener->SaveGameState(game_app->GetPlayerManager());
                }
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
            }
        };

        const auto address                = net::ip::make_address("0.0.0.0");
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

        // 7. Созраняем состояние игры при завершении работы программы
        if(serializer_listener) {
            serializer_listener->SaveGameState(game_app->GetPlayerManager());
        }
    } catch (const std::exception& ex) {
        log_server_exit_report["code"]      = EXIT_FAILURE;
        log_server_exit_report["exception"] = ex.what();
    }
    BOOST_LOG_TRIVIAL(info) << logging::add_value(log_message, "server exited")
                            << logging::add_value(log_msg_data, log_server_exit_report);
}

/**
 *Задание
Добавьте в игровой сервер возможность сохранять игровое состояние в файл и восстанавливать состояние из файла при старте.
Добавьте следующие параметры командной строки:
Параметр --state-file <путь-к-файлу> задаёт путь к файлу, в который приложение должно сохранять своё состояние в процессе работы,
а при старте — восстанавливать. Если параметр не задан, игровой сервер всегда запускается с чистого листа и не сохраняет своё состояние
в файл. Когда параметр задан и сервер завершает работу, получив сигналы SIGINT и SIGTERM, программа должна сохранить на диск своё текущее состояние.
Параметр --save-state-period <игровое-время-в-миллисекундах>. Задаёт период автоматического сохранения состояния сервера. Сохранение должно
происходить синхронно с ходом игровых часов, если с момента предыдущего сохранения состояния прошло не меньше указанного времени. Если
параметр не задан, состояние должно сохраняться автоматически только перед завершением работы сервера, когда ему отправлены сигналы SIGINT
и SIGTERM. Этот параметр игнорируется, если запустить сервер без параметра --state-file.
Когда в командной строке сервера указан параметр --state-file и по указанному пути есть файл, сервер должен восстановить своё состояние
из этого файла. Если при восстановлении состояния возникла ошибка, например, в файле есть некорректные данные, сервер должен:
в log вывести сообщение об ошибке;
завершить работу и вернуть из функции код ошибки EXIT_FAILURE.
Если файл, путь к которому задан в --state-file, на диске не существует, сервер должен начать работу с чистого листа.
Сервер должен использовать игровое время для определения моментов автоматического сохранения состояния. То есть сохранение должно происходить
в те моменты, когда происходит обновление игровых часов по HTTP-запросу /api/v1/game/tick либо автоматически, если сервер был запущен
с параметром командой строки --tick-period.
Состояние, которое сохраняется на диск, должно включать:
информацию обо всех динамических объектах на всех картах — собаках и потерянных предметах;
информацию о токенах и идентификаторах пользователей, вошедших в игру.
В каталоге sprint4/state_serialization/precode вы найдёте:
пример кода, который иллюстрирует сохранение и восстановление состояния нашей версии класса Dog,
заготовку юнит-тестов, которые проверяют загрузку и сериализацию.
Как будет тестироваться ваш код
Автоматические тесты проверят следующие сценарии:
Когда сервер запускается без указания пути к файлу с сохранённым состоянием, он должен стартовать с чистого листа. При получении сигнала о завершении работы сервер не должен создавать никаких файлов.
Когда сервер запускается с указанием пути к отсутствующему файлу состояния, он должен стартовать с чистого листа. При получении сигнала о завершении работы сервер должен сохранить состояние в файл.
Когда сервер запускается с указанием пути к существующему файлу состояния, он должен должен восстановить это состояние. При получении сигнала о завершении работы сервер должен сохранить обновлённое состояние.
Тесты не будут проверять содержимое файла — формат может быть любым. Но ваш сервер должен успешно восстанавливать игровое состояние из сохранённого файла.
При запуске с параметром --save-state-period сервер должен сохранять в него состояние, когда с момента предыдущего сохранения или старта игры прошёл интервал времени, не меньший заданного. Если время на сервере идёт автоматически (сервер запущен с параметром --tick-period), сохранение должно происходить синхронно с ходом игровых часов. Если сервер запущен без параметра --tick-period, сохранение должно происходить синхронно с обработкой запросов /api/v1/game/tick.
*/
