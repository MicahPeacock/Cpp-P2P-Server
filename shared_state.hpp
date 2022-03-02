#ifndef SHARED_STATE_HPP
#define SHARED_STATE_HPP

#include "net/socket_address.hpp"
#include "utils.hpp"

#include <mutex>
#include <unordered_map>

class shared_state {
public:
    using address_type = net::address_v4;
    using peer_type    = net::address_v4;
    using time_type    = clocks::time_type;

    using peer_map = std::unordered_map<peer_type, time_type>;

    explicit shared_state(const address_type& address)
            : m_address(address), m_is_running(true), m_timestamp(0) {}

    const address_type& address() const noexcept { return m_address; }
    const peer_map& peers() const noexcept { return m_peers; }
    size_t timestamp() const noexcept { return m_timestamp; }
    bool is_running() const noexcept { return m_is_running; }

    void join(const peer_type& peer) {
        std::scoped_lock lock(m_mutex);
        m_peers[peer] = clocks::get_current_time();
    }
    void leave(const peer_type& peer) {
        std::scoped_lock lock(m_mutex);
        m_peers.erase(peer);
    }
    void update(const peer_type& peer) {
        std::scoped_lock lock(m_mutex);
        m_peers[peer] = clocks::get_current_time();
    }

    void increment_timestamp() {
        std::scoped_lock lock(m_mutex);
        m_timestamp += 1;
    }
    void update_timestamp(size_t val) {
        std::scoped_lock lock(m_mutex);
        m_timestamp = std::max(m_timestamp, val);
    }

    void halt() {
        std::scoped_lock lock(m_mutex);
        m_is_running = false;
    }

private:
    const net::address_v4 m_address;

    std::unordered_map<peer_type, time_type> m_peers;
    std::mutex m_mutex;

    size_t m_timestamp;
    bool m_is_running;
};

#endif //SHARED_STATE_HPP
