#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

class AuthorRepositoryImpl final : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Author& author) override;
    std::optional<domain::AuthorId> FindAuthorByName(std::string author_name) const override;
    std::vector<std::pair<std::string, std::string>> GetAllAuthors() override;

    bool DeleteAuthor (const std::string& author_id) override;
    bool EditAuthorName (const std::string& author_id, const std::string& new_name) override;

private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl final : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Book& book) override;
    std::vector<std::pair<std::string, int>> GetAllBooks() const override;
    std::vector<std::pair<std::string, int>> GetAllBooksByAuthor(const std::string& author_id) const override;

    domain::Book LoadBook(const std::string& book_id) const override;
    bool DeleteBook(const std::string& book_id) override;
    bool ModifyBookInfo(const std::string& book_id, const std::vector<std::string>& tags_, std::string new_title = "", int new_pub_year = -1) override;

private:
    pqxx::connection& connection_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

    BookRepositoryImpl& GetBooks() & {
        return books_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl books_{connection_};
};

}  // namespace postgres