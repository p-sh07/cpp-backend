#pragma once

#include <string>
#include <vector>
#include <optional>

namespace app {
struct BookInfo {
    std::string title;
    std::string author_name;
    int publication_year;
    std::vector<std::string> tags_;
};

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(const std::string& title, const std::string& author_id, int publication_year) = 0;

    virtual BookInfo GetBookInfo(const std::string& book_id) const = 0;
    virtual std::optional<std::string> GetAuthorId(const std::string& author_name) const = 0;

    virtual std::vector<std::pair<std::string, int>> GetAllBooks() const = 0;
    virtual std::vector<std::pair<std::string, int>> GetBooksByAuthor(const std::string& author_id) const = 0;
    virtual std::vector<std::pair<std::string, std::string>> GetAllAuthors() const = 0;

    virtual void AddBookWithAuthor(const std::string& author_name, const std::string& book_title, int publication_year) = 0;
    virtual void DeleteAuthor(const std::string& author_id) = 0;
    virtual void DeleteBook(const std::string& book_id) = 0;
    virtual void EditAuthorName(const std::string& author_id, const std::string& new_name) = 0;
    virtual void EditBook(const std::string& book_id, const std::string& new_title = "", int new_pub_year = -1) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
