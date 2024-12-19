#include "postgres.h"

#include <pqxx/zview.hxx>
#include <pqxx/pqxx>
#include <pqxx/internal/result_iterator.hxx>
#include <stdexcept>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    try {
        pqxx::work work{connection_};
        work.exec_params(R"(
            INSERT INTO authors (id, name) VALUES ($1, $2)
            ON CONFLICT (id) DO UPDATE SET name=$2;
        )"_zv, author.GetId().ToString(), author.GetName()
        );
        work.commit();
    } catch (std::exception&) {
        throw std::runtime_error("failed to add author");
    }
}

std::vector<std::pair<std::string, std::string>> AuthorRepositoryImpl::GetAllAuthors() {
    pqxx::read_transaction read{connection_};
    std::vector<std::pair<std::string, std::string>> result;
    for(auto [id, name] : read.query<std::string, std::string>("SELECT * FROM authors ORDER BY name;"_zv)) {
        result.emplace_back(id, name);
    }
    return result;
}

bool AuthorRepositoryImpl::DeleteAuthor(const std::string& author_id) {
    pqxx::work work{connection_};
    try {
        //Delete book and tags inside a transaction
        work.exec(R"(START TRANSACTION;)"_zv);
        work.exec("DELETE FROM authors WHERE id = " + work.quote(author_id) + ";");
        work.exec("DELETE FROM books WHERE author_id = " + work.quote(author_id) + ";");

        work.exec(R"(COMMIT)"_zv);
    } catch (std::exception&) {
        work.exec(R"(ROLLBACK;)"_zv);
        return false;
    }
    return true;
}

bool AuthorRepositoryImpl::EditAuthorName(const std::string& author_id, const std::string& new_name) {
    pqxx::work work{connection_};
    try {
        //Delete book and tags inside a transaction
        work.exec(R"(START TRANSACTION;)"_zv);
        work.exec("UPDATE books SET name = " + work.quote(new_name) + " WHERE id = " + work.quote(author_id) + ";");
        work.exec(R"(COMMIT)"_zv);
    } catch (std::exception&) {
        work.exec(R"(ROLLBACK;)"_zv);
        return false;
    }
    return true;
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

domain::Book BookRepositoryImpl::LoadBook(const std::string& book_id) const {

}

bool BookRepositoryImpl::DeleteBook(const std::string& book_id) {
    pqxx::work work{connection_};
    try {
        //Delete book and tags inside a transaction
        work.exec(R"(START TRANSACTION;)"_zv);
        work.exec("DELETE FROM books WHERE id = " + work.quote(book_id) + ";");
        work.exec("DELETE FROM book_tags WHERE book_id = " + work.quote(book_id) + ";");
        work.exec(R"(COMMIT)"_zv);
        //TODO: linking the TAGS table with books could simplify this
    } catch (std::exception&) {
        work.exec(R"(ROLLBACK;)"_zv);
        return false;
    }
    return true;
}

bool BookRepositoryImpl::ModifyBookInfo(const std::string& book_id, const std::vector<std::string>& tags_, std::string new_title, int new_pub_year) {
    //TODO:
    // pqxx::work work{connection_};
    // try {
    //     //Delete book and tags inside a transaction
    //     work.exec(R"(START TRANSACTION;)"_zv);
    //     work.exec("DELETE FROM books WHERE id = " + work.quote(book_id) + ";");
    //     work.exec("DELETE FROM book_tags WHERE book_id = " + work.quote(book_id) + ";");
    //     work.exec(R"(COMMIT)"_zv);
    //     //TODO: linking the TAGS table with books could simplify this
    // } catch (std::exception&) {
    //     work.exec(R"(ROLLBACK;)"_zv);
    //     return false;
    // }
    return false;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
        CREATE TABLE IF NOT EXISTS authors (
            id UUID CONSTRAINT author_id_constraint PRIMARY KEY NOT NULL,
            name varchar(100) UNIQUE NOT NULL
        );
    )"_zv);

    work.exec(R"(
        CREATE TABLE IF NOT EXISTS books (
            id UUID CONSTRAINT book_id_constraint PRIMARY KEY NOT NULL,
            author_id UUID CONSTRAINT author_id_constraint NOT NULL,
            title varchar(100) NOT NULL, publication_year integer NOT NULL
        );
    )"_zv);

    work.exec(R"(
        CREATE TABLE IF NOT EXISTS book_tags (
            entry_id SERIAL PRIMARY KEY NOT NULL,
            book_id UUID CONSTRAINT book_id_constraint NOT NULL,
            tag varchar(30) NOT NULL
        );
    )"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres