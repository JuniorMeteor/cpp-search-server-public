#pragma once

#include <chrono>
#include <iostream>
#include <typeinfo>

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
#define LOG_DURATION_STREAM(x, stream) LogDuration UNIQUE_VAR_NAME_PROFILE(x, stream)

class LogDuration {
public:
    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string& id, std::ostream& os = std::cerr)
        : id_(id), os_(os) {
    }

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        // надо как то сделать ветвление на вывод инфы в cerr и в cout сейчас это говнище дублируется, если передать cout
        os_ << id_ << ": "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
        std::cout << "Operation time: "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    std::ostream& os_;
    const std::string id_;
    const Clock::time_point start_time_ = Clock::now();
};