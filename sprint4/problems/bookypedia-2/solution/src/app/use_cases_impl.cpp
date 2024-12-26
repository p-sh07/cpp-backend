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

void UseCasesImpl::AddBookWithAuthor(const std::string& author_name, const std::string& book_title, int publication_year) {
    //TODO:
}

void UseCasesImpl::DeleteAuthor(const std::string& author_id) {
    //TODO:
}

void UseCasesImpl::EditAuthorName(const std::string& author_id, const std::string& new_name) {
    //TODO:
}

void UseCasesImpl::DeleteBook(const std::string& book_id) {
    //TODO:
}

void UseCasesImpl::EditBook(const std::string& book_id, const std::string& new_title, int new_pub_year) {
    //TODO:
}

std::vector<std::pair<std::string, std::string>> UseCasesImpl::GetAllAuthors() const {
    return authors_.GetAllAuthors();
}

std::vector<std::pair<std::string, int>> UseCasesImpl::GetAllBooks() const {
    return books_.GetAllBooks();
}

BookInfo UseCasesImpl::GetBookInfo(const std::string& book_id) const {
    //TODO:
    auto book = books_.LoadBook(book_id);
}

std::optional<std::string> UseCasesImpl::GetAuthorId(const std::string& author_name) const {
    auto id = authors_.FindAuthorByName(author_name);
    if(!id) {
        return std::nullopt;
    }
    return {id.value().ToString()};
}

std::vector<std::pair<std::string, int>> UseCasesImpl::GetBooksByAuthor(const std::string& author_id) const {
    return books_.GetAllBooksByAuthor(author_id);
}
}  // namespace app
