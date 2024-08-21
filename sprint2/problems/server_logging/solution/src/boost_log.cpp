//
// Created by Pavel on 19.08.2024.
//

#include "boost_log.h"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace date_time = boost::posix_time;

void bstlog::InitLogging() {
    logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);
    logging::add_common_attributes();
    logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());

    logging::add_console_log(
            std::clog,
            keywords::format = &JsonFormatter,
            keywords::auto_flush = true
    );

    //     logging::add_file_log(
    //         // NB: -> Add file timestamp
    //         keywords::file_name = "server_%N.log",
    //         keywords::format = &bstlog::JsonFormatter,
    //         keywords::open_mode = std::ios_base::app | std::ios_base::out,
    //         keywords::rotation_size = 10 * 1024 * 1024,
    //         keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(12, 0, 0)
    // );
}

void bstlog::JsonFormatter(logging::record_view const &rec, logging::formatting_ostream &strm) {
    json::object msg;

    auto ts = rec[timestamp];
    msg["timestamp"] = to_iso_extended_string(*ts);

    msg["data"] = *rec[log_msg_data];
    msg["message"] = *rec[log_message];

    strm << msg;
}