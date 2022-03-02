#ifndef IO_CONTEXT_HPP
#define IO_CONTEXT_HPP

#include <string>
#include <queue>
#include <mutex>

namespace net {

class io_context {
public:
    [[nodiscard]] const std::queue<std::string>& incoming() const noexcept {
        return m_incoming;
    }

    [[nodiscard]] bool has_incoming() const noexcept {
        return !m_incoming.empty();
    }

    void put_incoming(const std::string& message) noexcept {
        std::scoped_lock lock(m_mutex);
        m_outgoing.push(message);
    }

    std::string pop_incoming() noexcept {
        std::scoped_lock lock(m_mutex);
        auto ret = m_outgoing.front();
        m_outgoing.pop();
        return ret;
    }

    [[nodiscard]] const std::queue<std::string>& outgoing() const noexcept {
        return m_outgoing;
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
    std::queue<std::string> m_incoming;
    std::queue<std::string> m_outgoing;

    std::mutex m_mutex;
};

} // net

#endif //IO_CONTEXT_HPP
