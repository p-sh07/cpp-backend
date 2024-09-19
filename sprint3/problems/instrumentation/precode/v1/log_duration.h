//
//  05.Производительность и оптимизация
//  Тема 1.Профилируем и ускоряем
//  Задача 05.Используем макрос
//
//  Created by Pavel Sh on 23.12.2023.
//

#pragma once

#include <chrono>
#include <iostream>
#include <string>

#define UNIQUE_ID_IMPL(lineno) profile_guard_##lineno
#define UNIQUE_ID(lineno) UNIQUE_ID_IMPL(lineno)
#define LOG_DURATION(message) LogDuration UNIQUE_ID(__LINE__){message};

class LogDuration {
public:
    using Clock = std::chrono::steady_clock;
    
    LogDuration() {}
    
    explicit LogDuration(const std::string& message)
    : message_(message) {
    }

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;
       
        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        if(!message_.empty()) {
            std::cerr << message_ << ": "s;
        }
        std::cerr << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
   }

private:
    const Clock::time_point start_time_ = Clock::now();
    std::string message_;
};


