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