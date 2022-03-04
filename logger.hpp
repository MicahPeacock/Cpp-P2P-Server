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
    const std::string date = clocks::get_current_time_str();
};


class logger {
public:
    using peer_list_t = std::unordered_map<log_type, std::vector<log_entry>>;
/**
     *
     * @return
     */
    template<log_type Type>
    [[nodiscard]] const std::vector<log_entry>& log() const {
        return m_logs->at(Type);
    }

    /**
     *
     * @param msg
     */
    template<log_type Type>
    void log(const std::string& msg) {
        std::scoped_lock lock(m_mutex);
        m_logs->operator[](Type).emplace_back(msg);
    }

    std::vector<log_entry>& operator[] (log_type log_type) {
        return m_logs->operator[](log_type);
    }

    const std::vector<log_entry>& operator[] (log_type log_type) const {
        return m_logs->at(log_type);
    }

private:
    std::shared_ptr<peer_list_t> m_logs = std::make_shared<peer_list_t>();
    std::mutex m_mutex;
};

#endif // LOGGER_HPP
