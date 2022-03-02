#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <chrono>
#include <algorithm>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <numeric>
#include <mutex>

#include "utils.hpp"

using namespace std::chrono;

enum class log_type {
    peers,
    sources,
    send_peers,
    recv_peers,
    snippets,
};

struct log_entry {
    std::string message;
    std::string date = clocks::get_current_time_str();
};


class logger {
public:
    std::vector<log_entry>& operator[] (log_type type) {
        return m_logs[type];
    }

    const std::vector<log_entry>& operator[] (log_type type) const {
        return m_logs.at(type);
    }

    template<log_type Type>
    void operator()(const std::string& msg) {
        std::scoped_lock lock(m_mutex);
        m_logs[Type].emplace_back(msg);
    }

    template<log_type Type>
    std::vector<log_entry>& operator()() {
        return m_logs[Type];
    }

    template<log_type Type>
    const std::vector<log_entry>& operator()() const {
        return m_logs.at(Type);
    }

private:
    std::unordered_map<log_type, std::vector<log_entry>> m_logs;
    std::mutex m_mutex;
};

#endif // LOGGER_HPP
