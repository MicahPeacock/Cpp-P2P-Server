#ifndef IO_CONTEXT_HPP
#define IO_CONTEXT_HPP

#include <queue>
#include <string>
#include <mutex>

namespace net {

struct message {
    std::string sender;
    std::string content;
    size_t timestamp;
};

inline std::ostream& operator<<(std::ostream& os, const message& msg) {
    return (os << msg.timestamp << ' ' << msg.sender << "> " << msg.content);
}


/**
 *
 */
class io_context {
public:
    [[nodiscard]] bool has_incoming() const noexcept {
        return !m_incoming.empty();
    }

    void put_incoming(const net::address_v4& sender, const std::string& message, size_t timestamp) noexcept {
        std::scoped_lock lock(m_mutex);
        m_incoming.push({ sender.to_string(), message, timestamp });
    }

    net::message pop_incoming() noexcept {
        std::scoped_lock lock(m_mutex);
        auto ret = m_incoming.front();
        m_incoming.pop();
        return ret;
    }


    [[nodiscard]] bool has_outgoing() const noexcept {
        return !m_outgoing.empty();
    }

    void put_outgoing(const std::string& message) noexcept {
        std::scoped_lock lock(m_mutex);
        m_outgoing.push(message);
    }

    std::string pop_outgoing() noexcept {
        std::scoped_lock lock(m_mutex);
        auto ret = m_outgoing.front();
        m_outgoing.pop();
        return ret;
    }

private:
    std::queue<net::message> m_incoming;
    std::queue<std::string> m_outgoing;

    std::mutex m_mutex;
};

} // net

#endif //IO_CONTEXT_HPP
