#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/log/utility/setup/file.hpp>  //для вывода лога в файл
#include <boost/log/utility/setup/common_attributes.hpp> //для вывода плейсхолдеров %LineID% %TimeStamp% etc
#include <boost/log/utility/setup/console.hpp> //для вывода в консоль

#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/date_time.hpp>       // для вывода даты

#include <string_view>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;

using namespace std::literals;

 BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
 BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

 void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
     // Выводить LineID стало проще.
      strm << rec[line_id] << ": ";

     // Момент времени приходится вручную конвертировать в строку.
     // Для получения истинного значения атрибута нужно добавить
     // разыменование.
      auto ts = *rec[timestamp];
      strm << to_iso_extended_string(ts) << ": ";

     // Выводим уровень, заключая его в угловые скобки.
     strm << "<" << rec[logging::trivial::severity] << "> ";

     // Выводим само сообщение.
     strm << rec[logging::expressions::smessage];
 }

void InitLogging() {
    // logging::add_console_log(
    //     std::clog,
    //     keywords::format = "[%TimeStamp%]: %Message%",
    //     keywords::auto_flush = true
    //     );
    //

    // logging::add_file_log(
    //     keywords::file_name = "server_%N.log",
    //     keywords::format = "[%TimeStamp%]: %Message%",
    //     keywords::open_mode = std::ios_base::app | std::ios_base::out,
    //     keywords::rotation_size = 10 * 1024 * 1024,
    //     keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(12, 0, 0)
    //     );
}



int main() {
    InitLogging();

    BOOST_LOG_TRIVIAL(trace) << "Сообщение уровня trace"sv;
    BOOST_LOG_TRIVIAL(debug) << "Сообщение уровня debug"sv;
    BOOST_LOG_TRIVIAL(info) << "Сообщение уровня info"sv;
    BOOST_LOG_TRIVIAL(warning) << "Сообщение уровня warning"sv;
    BOOST_LOG_TRIVIAL(error) << "Сообщение уровня error"sv;
    BOOST_LOG_TRIVIAL(fatal) << "Сообщение уровня fatal"sv;
}
