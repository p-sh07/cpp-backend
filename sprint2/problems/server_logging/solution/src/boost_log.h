//
// Created by Pavel on 19.08.2024.
//
#pragma once
#include <boost/json.hpp>
#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/date_time.hpp>       // для вывода даты
#include <boost/log/utility/setup/file.hpp>  //для вывода лога в файл
#include <boost/log/utility/setup/common_attributes.hpp> //для вывода плейсхолдеров %LineID% %TimeStamp% etc
#include <boost/log/utility/setup/console.hpp> //для вывода в консоль
#include <boost/log/utility/manipulators/add_value.hpp>

#include <string_view>
#include "json_loader.h"

//Что бы упростить конструкцию с logging::extract
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

BOOST_LOG_ATTRIBUTE_KEYWORD(log_message, "message", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(log_msg_data, "data", boost::json::value)

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;

namespace log {
    using namespace std::literals;

    void InitLogging();
    void JsonFormatter(logging::record_view const &rec, logging::formatting_ostream &strm);
}