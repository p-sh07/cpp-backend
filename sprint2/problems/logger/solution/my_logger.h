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

        //std::cerr << "logging msg to file: " << MakeLogFileName() << std::endl;
        log_file_.open(MakeLogFileName(), std::ios_base::app);

        if(!log_file_.is_open()) {
            throw std::runtime_error("Unable to open log file for writing!");
        }

        log_file_ << GetTimeStamp() << ": "sv ;
        ((log_file_ << args), ...);
        log_file_ << std::endl;

        log_file_.close();
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> lock(m_);
        manual_ts_.emplace(ts);
    }

private:
    std::mutex m_;
    std::optional<std::chrono::system_clock::time_point> manual_ts_;

    std::ofstream log_file_{MakeLogFileName(), std::ios_base::app};

    std::string MakeLogFileName() const {
        return "/var/log/sample_log_" + GetFileTimeStamp() + ".log";
    }
};
