#include <gtest/gtest.h>
#include <vector>
#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, EncodingTests) {
// Пустая входная строка
    EXPECT_EQ(UrlEncode(""sv), ""s);
// Входная строка без служебных символов
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
// Входная строка со служебными символами
    EXPECT_EQ(UrlEncode("some!#$&'()*symbols,/:;=?@[]"sv), "some%21%23%24%26%27%28%29%2asymbols%2c%2f%3a%3b%3d%3f%40%5b%5d"s);
// Входная строка с пробелами
    EXPECT_EQ(UrlEncode("hello space"sv), "hello+space"s);
// Входная строка с символами с кодами меньше 31 и большими или равными 128
    std::string border_cases{static_cast<char>(31),
                             static_cast<char>(32),
                             static_cast<char>(33),
                             static_cast<char>(127),
                             static_cast<char>(128)}; //-still gives error
    std::string result = "%1f+%21"s;
    result += static_cast<char>(127);
    result += "%80";

    EXPECT_EQ(UrlEncode(border_cases), result);
}
