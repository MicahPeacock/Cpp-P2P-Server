#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

#include "utils.hpp"


struct source_entry {
    std::vector<net::address_v4> peers;
    std::string date;
};

struct peer_entry {
    std::string to;
    std::string from;
    std::string date;
};

struct snippet_entry {
    size_t timestamp;
    std::string message;
    std::string sender;
};

class logger {
public:
    void log_peer(const std::string& peer) {
        std::scoped_lock lock(m_mutex);
        m_peers.insert(peer);
    }

    void log_source(const std::string& src, const std::vector<net::address_v4>& peers) {
        std::scoped_lock lock(m_mutex);
        m_sources[src] = { peers, clocks::get_current_time_str() };
    }

    void log_sent_peer(const std::string& to, const std::string& from) {
        std::scoped_lock lock(m_mutex);
        m_sent_peers.push_back({ to, from, clocks::get_current_time_str() });
    }

    void log_recv_peer(const std::string& to, const std::string& from) {
        std::scoped_lock lock(m_mutex);
        m_recv_peers.push_back({ to, from, clocks::get_current_time_str() });
    }

    void log_snippet(size_t timestamp, const std::string& snippet, const std::string& sender) {
        std::scoped_lock lock(m_mutex);
        m_snippets.push_back({ timestamp, snippet, sender });
    }

    const std::unordered_set<std::string>& peer_log() const noexcept {
        return m_peers;
    }

    const std::unordered_map<std::string, source_entry>& source_log() const noexcept {
        return m_sources;
    }

    const std::vector<peer_entry>& sent_peers_log() const noexcept {
        return m_sent_peers;
    }

    const std::vector<peer_entry>& recv_peers_log() const noexcept {
        return m_recv_peers;
    }

    const std::vector<snippet_entry>& snippet_log() const noexcept {
        return m_snippets;
    }

private:
    std::unordered_set<std::string> m_peers;
    std::unordered_map<std::string, source_entry> m_sources;
    std::vector<peer_entry> m_sent_peers;
    std::vector<peer_entry> m_recv_peers;
    std::vector<snippet_entry> m_snippets;

    std::mutex m_mutex;
};

#endif // LOGGER_HPP
