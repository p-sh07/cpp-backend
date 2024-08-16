#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>

using namespace std::literals;
using namespace std::chrono;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        const auto now = GetTime();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y_%m_%d");
        return ss.str();
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard<std::mutex> lock (m_);

        auto current_date = GetFileTimeStamp();
        if(IsNewDate(current_date)) {
            log_file_.close();
            log_file_.open(MakeLogFilename(current_date), std::ios_base::app);
        }

        if(!log_file_.is_open()) {
            throw std::runtime_error("Unable to open log file for writing: [" + date_string_ + "]");
        }

        log_file_ << GetTimeStamp() << ": "sv ;
        ((log_file_ << args), ...);
        log_file_ << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> lock(m_);
        manual_ts_.emplace(ts);
    }

private:
    mutable std::mutex m_;
    std::optional<std::chrono::system_clock::time_point> manual_ts_;

    std::string date_string_ = GetFileTimeStamp();
    std::ofstream log_file_{MakeLogFilename(date_string_), std::ios_base::app};

    bool IsNewDate(std::string date_str) {
        return date_string_ != date_str;
    }

    std::string MakeLogFilename(const std::string& date_str) {
        return "/var/log/sample_log_" + date_str + ".log";
    }
};
