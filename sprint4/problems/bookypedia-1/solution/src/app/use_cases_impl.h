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

    std::string GetAuthorId(const std::string& author_name) override;
    std::vector<std::pair<std::string, std::string>> GetAllAuthors() override;

    std::vector<std::pair<std::string, int>> GetAllBooks() override;

    std::vector<std::pair<std::string, int>> GetBooksByAuthor(const std::string& author_id) override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app
