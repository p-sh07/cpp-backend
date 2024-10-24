#include <iostream>

#include "leap_year.h"

int main() {
    using namespace std::literals;

    std::cout << "Enter year: "sv;

    int year;
    std::cin >> year;

    std::cout << year << " is a "sv << (IsLeapYear(year) ? "leap"sv : "non-leap"sv) << " year"sv
              << std::endl;
}