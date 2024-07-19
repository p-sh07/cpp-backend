#pragma once
#include <boost/test/unit_test.hpp>

namespace test_util {

inline std::ostream& operator<<(std::ostream& out, std::nullopt_t) {
    using namespace std::literals;
    return out << "nullopt"sv;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& out, std::optional<T> const& opt_value) {
    using namespace std::literals;
    if (opt_value) {
        return out << *opt_value;
    } else {
        return out << std::nullopt;
    }
}

}  // namespace test_util

namespace boost::test_tools::tt_detail {

template <>
struct print_log_value<std::nullopt_t> {
    void operator()(std::ostream& out, const std::nullopt_t&) {
        using ::test_util::operator<<;
        out << std::nullopt;
    }
};

template <typename T>
struct print_log_value<std::optional<T>> {
    void operator()(std::ostream& out, const std::optional<T>& opt_value) {
        using ::test_util::operator<<;
        out << opt_value;
    }
};

}  // namespace boost::test_tools::tt_detail
