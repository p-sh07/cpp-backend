//
// Created by Pavel on 19.08.2024.
//

#include "server_logger.h"

namespace json = boost::json;
namespace keywords = boost::log::keywords;
namespace attrs = boost::log::attributes;

void server_logger::InitLogging() {
    logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);
    logging::add_common_attributes();
    //logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());

    logging::add_console_log(
        std::clog,
        keywords::format = &JsonFormatter,
        keywords::auto_flush = true
    );

#ifdef ENABLE_FILE_LOG
    logging::add_file_log(
        // NB: -> Add file timestamp
        keywords::file_name = "server_%N.log",
        keywords::format = &bstlog::JsonFormatter,
        keywords::open_mode = std::ios_base::app | std::ios_base::out,
        keywords::rotation_size = 10 * 1024 * 1024,
        keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(12, 0, 0)
);
#endif
}

void server_logger::JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    json::object msg;

    auto ts = rec[timestamp];
    msg["timestamp"] = to_iso_extended_string(*ts);

    msg["data"] = *rec[log_msg_data];
    msg["message"] = *rec[log_message];

    strm << msg;
}