#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

// Пустая строка
    BOOST_TEST(UrlDecode(""sv) == ""s);

// Строка без %-последовательностей.
    BOOST_TEST(UrlDecode("just_a_string"sv) == "just_a_string"s);

// Строка с валидными %-последовательностями, записанными в разном регистре.
    BOOST_TEST(UrlDecode("some%2a%21%23%24%26%27symbols%2B%28%40%5d"sv) == "some*!#$&'symbols+(@]"s);

// Строка с невалидными %-последовательностями.
    BOOST_CHECK_THROW(UrlDecode("invalid%3G"sv), std::invalid_argument);
    //BOOST_CHECK_THROW(UrlDecode("invalid%F9"sv), std::invalid_argument);
    // Other invalid % sequence ?

// Строка с неполными %-последовательностями.
    BOOST_CHECK_THROW(UrlDecode("invalid%3-other+text"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("invalid%2"sv), std::invalid_argument);
// Строка с символом +.
    BOOST_TEST(UrlDecode("a+space"sv) == "a space"s);
}



/* Example:
 *
#define BOOST_TEST_MODULE leap year application tests
#include <boost/test/unit_test.hpp>

#include "../src/leap_year.h"

BOOST_AUTO_TEST_CASE(IsLeapYear_test) {
    BOOST_CHECK(IsLeapYear(2020));
    BOOST_CHECK(!IsLeapYear(2021));
    BOOST_CHECK(!IsLeapYear(2022));
    BOOST_CHECK(!IsLeapYear(2023));
    BOOST_CHECK(IsLeapYear(2024));
    BOOST_CHECK(!IsLeapYear(1900));
    BOOST_CHECK(IsLeapYear(2000));
}

 int Sqr(int x) {
    return x * x;
}

BOOST_AUTO_TEST_CASE(Sqr_test) {
    BOOST_CHECK_EQUAL(Sqr(3), 9);
    BOOST_CHECK_EQUAL(Sqr(-5), 25);
}

std::vector<int> GetEvenNumbers(const std::vector<int> numbers) {
    std::vector<int> even_numbers;
    even_numbers.reserve(numbers.size());
    std::copy_if(                          //
        numbers.begin(), numbers.end(),    //
        std::back_inserter(even_numbers),  //
        [](int number) {
            return number % 2 == 0;
        });
    return even_numbers;
}

BOOST_AUTO_TEST_CASE(GetEvenNumbers_test) {
    auto evens = GetEvenNumbers({1, 2, 3, 4, 5});
    BOOST_REQUIRE_EQUAL(evens.size(), 2);
    // Если предыдущая проверка провалится, то тест прервётся
    // и обращения к несуществующим элементам вектора не произойдёт

    BOOST_CHECK_EQUAL(evens[0], 2);
    BOOST_CHECK_EQUAL(evens[1], 4);
}

BOOST_AUTO_TEST_CASE(boost_test_example) {
    // Будет выведено значение каждого операнда выражения
    BOOST_TEST(Sqr(3) + Sqr(4) == 24);
    // В случае ошибки будет выведено выражение в скобках целиком
    BOOST_TEST((Sqr(3) + Sqr(4)) == 24);

    std::string str{"123"};
    BOOST_TEST_REQUIRE(!str.empty()); // Если условие не выполнится, test case прервёт работу
    BOOST_TEST(str[0] == 'a');
}

 */

//reserved: !#$&'()*+,/:;=?@[] -> encoded
//non-encoded: -_.~
//space is +