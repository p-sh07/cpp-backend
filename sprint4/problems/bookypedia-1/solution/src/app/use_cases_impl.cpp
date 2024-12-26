#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

#include <stdexcept>

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(const std::string& title, const std::string& author_id, int publication_year) {
    books_.Save({BookId::New(), domain::AuthorId::FromString(author_id), title, publication_year});
}

std::vector<std::pair<std::string, std::string>> UseCasesImpl::GetAllAuthors() {
    return authors_.GetAllAuthors();
}

std::vector<std::pair<std::string, int>> UseCasesImpl::GetAllBooks() {
    return books_.GetAllBooks();
}

std::string UseCasesImpl::GetAuthorId(const std::string& author_name) {
    auto id = authors_.FindAuthorByName(author_name);
    if(!id) {
        throw std::out_of_range("Author not found by name");
    }
    return id.value().ToString();
}

std::vector<std::pair<std::string, int>> UseCasesImpl::GetBooksByAuthor(const std::string& author_id) {
    return books_.GetAllBooksByAuthor(author_id);
}
}  // namespace app
