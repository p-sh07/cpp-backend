#include "postgres.h"

#include <pqxx/zview.hxx>
#include <pqxx/pqxx>
#include <pqxx/internal/result_iterator.hxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    work.exec_params(R"(
            INSERT INTO authors (id, name) VALUES ($1, $2)
            ON CONFLICT (id) DO UPDATE SET name=$2;
        )"_zv, author.GetId().ToString(), author.GetName()
    );
    work.commit();
}

std::vector<std::pair<std::string, std::string>> AuthorRepositoryImpl::GetAllAuthors() {
    pqxx::read_transaction read{connection_};
    std::vector<std::pair<std::string, std::string>> result;
    for(auto [id, name] : read.query<std::string, std::string>("SELECT * FROM authors ORDER BY name;"_zv)) {
        result.emplace_back(id, name);
    }
    return result;
}

std::optional<domain::AuthorId> AuthorRepositoryImpl::FindAuthorByName(std::string author_name) const {
    pqxx::read_transaction read{connection_};
    try {
        return domain::AuthorId::FromString(
            read.query_value<std::string>(
                "SELECT id FROM authors WHERE name =" + read.quote(author_name) + ";"
            )
        );
    } catch (std::exception& ex) {
        return std::nullopt;
    }
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec_params(R"(
            INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4);
        )"_zv, book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetPublicationYear()
    );
    work.commit();
}

std::vector<std::pair<std::string, int>> BookRepositoryImpl::GetAllBooks() const {
    pqxx::read_transaction read{connection_};
    std::vector<std::pair<std::string, int>> result;

    for(auto const&[title, pub_year] : read.query<std::string, int>("SELECT title, publication_year FROM books ORDER BY title"_zv)) {
        result.emplace_back(title, pub_year);
                                                                    }
    return result;
}

std::vector<std::pair<std::string, int>> BookRepositoryImpl::GetAllBooksByAuthor(const std::string& author_id) const {
    pqxx::read_transaction read{connection_};
    std::vector<std::pair<std::string, int>> result;

    for(auto const&[title, pub_year] : read.query<std::string, int>("SELECT title, publication_year FROM books WHERE author_id = "
                                                                    + read.quote(author_id) + "ORDER BY publication_year, title")) {
        result.emplace_back(title, pub_year);
    }
    return result;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
        CREATE TABLE IF NOT EXISTS authors (
            id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
            name varchar(100) UNIQUE NOT NULL
        );
    )"_zv);

    work.exec(R"(
        CREATE TABLE IF NOT EXISTS books (
            id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
            author_id UUID CONSTRAINT author_id_constraint NOT NULL,
            title varchar(100) NOT NULL, publication_year integer NOT NULL
        );
    )"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres