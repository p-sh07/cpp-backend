#pragma once

#include <string>
#include <vector>

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(const std::string& title, const std::string& author_id, int publication_year) = 0;

    virtual std::string GetAuthorId(const std::string& author_name) = 0;
    virtual std::vector<std::pair<std::string, std::string>> GetAllAuthors() = 0;

    virtual std::vector<std::pair<std::string, int>> GetAllBooks() = 0;
    virtual std::vector<std::pair<std::string, int>> GetBooksByAuthor(const std::string& author_id) = 0;
protected:
    ~UseCases() = default;
};

}  // namespace app
