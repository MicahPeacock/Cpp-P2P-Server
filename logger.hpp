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
    /**
     *
     * @return
     */
    template<log_type LogType>
    [[nodiscard]] const std::vector<log_entry>& log() const {
        return m_logs.at(LogType);
    }

    /**
     *
     * @param msg
     */
    template<log_type LogType>
    void log(const std::string& msg) {
        m_logs[LogType].emplace_back(msg);
    }

    std::vector<log_entry>& operator[] (log_type log_type) {
        return m_logs[log_type];
    }

    const std::vector<log_entry>& operator[] (log_type log_type) const {
        return m_logs.at(log_type);
    }

private:
    std::unordered_map<log_type, std::vector<log_entry>> m_logs;
    std::mutex m_mutex;
};

#endif // LOGGER_HPP
