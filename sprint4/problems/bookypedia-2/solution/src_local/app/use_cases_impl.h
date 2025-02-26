#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl final : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
        : authors_{authors}
        , books_{books} {
    }

    void AddAuthor(const std::string& name) override;
    void AddBook(const std::string& title, const std::string& author_id, int publication_year) override;
    void AddBookWithAuthor(const std::string& author_name, const std::string& book_title, int publication_year) override;

    void DeleteAuthor (const std::string& author_id) override;
    void EditAuthorName (const std::string& author_id, const std::string& new_name) override;
    void DeleteBook (const std::string& book_id) override;
    void EditBook (const std::string& book_id, const std::string& new_title = "", int new_pub_year = -1) override;

    BookInfo GetBookInfo(const std::string& book_id) const override;
    std::optional<std::string> GetAuthorId(const std::string& author_name) const override;
    std::vector<std::pair<std::string, int>> GetAllBooks() const override;
    std::vector<std::pair<std::string, int>> GetBooksByAuthor(const std::string& author_id) const override;
    std::vector<std::pair<std::string, std::string>> GetAllAuthors() const override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app
