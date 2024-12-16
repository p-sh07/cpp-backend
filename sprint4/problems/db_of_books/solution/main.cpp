// main.cpp
#include <boost/json.hpp>

#include <iostream>
#include <pqxx/pqxx>

using namespace std::literals;
namespace json = boost::json;

// libpqxx использует zero-terminated символьные литералы вроде "abc"_zv;
using pqxx::operator"" _zv;

int main(int argc, const char* argv[]) {
    try {
        if (argc == 1) {
            std::cout << "Usage: book_manager <conn-string>\n"sv;
            return EXIT_SUCCESS;
        } else if (argc != 2) {
            std::cerr << "Invalid command line\n"sv;
            return EXIT_FAILURE;
        }

        // Подключаемся к БД, указывая её параметры в качестве аргумента
        pqxx::connection conn{argv[1]};

        // Создаём транзакцию. Это понятие будет разобрано в следующих уроках.
        // Транзакция нужна, чтобы выполнять запросы.
        pqxx::work make_table(conn);

        // Используя транзакцию создадим таблицу в выбранной базе данных:
        make_table.exec(
            "CREATE TABLE IF NOT EXISTS books (id SERIAL PRIMARY KEY, title varchar(100) NOT NULL, author varchar(100) NOT NULL, year integer NOT NULL, ISBN char(13) UNIQUE);"_zv
            );

        make_table.commit();

        constexpr auto tag_add_book = "add_book"_zv;
        conn.prepare(tag_add_book, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4)"_zv);

        //request loop
        std::string request_str;
        while(std::getline(std::cin, request_str)) {
            json::object request_obj    = json::parse(request_str).as_object();
            std::string_view req_action = request_obj.at("action").as_string();

            pqxx::work wk(conn);

            if(req_action == "exit"sv) {
                break;
            } else if(req_action == "add_book"sv) {
                try {
                    json::object jbook = request_obj.at("payload").as_object();
                    const std::string_view title = jbook.at("title").as_string();
                    const std::string_view author = jbook.at("author").as_string();
                    const auto year = jbook.at("year").as_int64();
                    const std::optional<std::string> isbn =
                        jbook.at("ISBN").is_string() ? std::optional<std::string>{jbook.at("ISBN").as_string()}
                    : std::optional<std::string>{std::nullopt};

                    wk.exec_prepared(tag_add_book, title, author, year, isbn);
                    std::cout << "{\"result\":true}" << std::endl;
                } catch (std::exception& ex) {
                    std::cout << "{\"result\":false}" << std::endl;
                }
            } else if(req_action == "all_books"sv) {
                bool first = true;
                std::cout << '[';
                for(const auto& book : wk.exec("SELECT to_json(b) FROM books b;"_zv)) {
                    if(!first) {
                        std::cout << ',';
                    }
                    first = false;
                    std::cout << *book.begin();
                }
                std::cout << ']';
            } else {
                break;
            }
            // Применяем все изменения
            wk.commit();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

/**
 *
{"action":"add_book","payload":{"title":"The Old Man and the Sea","author":"Hemingway","year":1952,"ISBN":"5555555555555"}}
{"result":true}
{"action":"add_book","payload":{"title":"Anna Karenina","author":"Tolstoi","year":1878,"ISBN":null}}
{"result":true}
{"action":"add_book","payload":{"title":"The Twelve Chairs","author":"Ilf, Petrov","year":1928,"ISBN":null}}
{"result":true}
{"action":"add_book","payload":{"title":"The catcher in the Rye","author":"Salinger","year":1952,"ISBN":"0123456789012"}}
{"result":true}
{"action":"add_book","payload":{"title":"The Old Woman","author":"Harms","year":1939,"ISBN":"0123456789012"}}
{"result":false}
{"action":"all_books","payload":{}}
[{"id":12,"title":"The catcher in the Rye","author":"Salinger","year":1952,"ISBN":"0123456789012"},{"id":9,"title":"The Old Man and the Sea","author":"Hemingway","year":1952,"ISBN":"5555555555555"},{"id":11,"title":"The Twelve Chairs","author":"Ilf, Petrov","year":1928,"ISBN":null},{"id":10,"title":"Anna Karenina","author":"Tolstoi","year":1878,"ISBN":null}]
{"action":"exit","payload":{}}

*Реализуйте программу для заполнения базы данных книг с одной таблицей books с такими полями:
id — идентификатор (тип SERIAL),
title — название книги (тип varchar(100)),
author — имя автора (тип varchar(100)),
year — год издания (тип integer),
ISBN — регистрационный номер, значение должно быть уникальным (тип char(13)).

Первый вид запросов — добавление книги. Словарь с ключами:
action — строка “add_book”,
payload — словарь с ключами title, author, year, ISBN.
После выполнения запроса программа выводит в stdout объект JSON, который содержит одно поле result со значением true или false, говорящий об успехе операции.
Второй вид запросов — вывести все книги. Словарь с ключами:
action — строка “all_books”,
payload — пустой словарь.
После выполнения запроса программа выводит в stdout массив JSON. Каждый элемент массива — словарь со ключами id, title, author, year, ISBN.
Третий вид запросов — выход. Словарь с ключами:
action — строка “exit”,
payload — пустой словарь.
*/
