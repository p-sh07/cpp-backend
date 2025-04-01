#pragma once
#include <string>
#include <optional>
#include <vector>

#include "../util/tagged_uuid.h"

namespace domain {

namespace detail {
struct AuthorTag {};
}  // namespace detail

using AuthorId = util::TaggedUUID<detail::AuthorTag>;

class Author {
public:
    Author(AuthorId id, std::string name)
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const AuthorId& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

private:
    AuthorId id_;
    std::string name_;
};

class AuthorRepository {
public:
    virtual void Save(const Author& author) = 0;
    virtual std::optional<AuthorId> FindAuthorByName(std::string author_name) const = 0;
    virtual std::vector<std::pair<std::string, std::string>> GetAllAuthors() = 0;
    virtual bool DeleteAuthor (const std::string& author_id) = 0;
    virtual bool EditAuthorName (const std::string& author_id, const std::string& new_name) = 0;
protected:
    ~AuthorRepository() = default;
};

}  // namespace domain
