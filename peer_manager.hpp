#ifndef PEER_MANAGER_HPP
#define PEER_MANAGER_HPP

#include "net/udp.hpp"
#include "net/buffer.hpp"
#include "logger.hpp"
#include "shared_state.hpp"
#include "io_context.hpp"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <numeric>
#include <thread>

using namespace std::chrono;

auto parse_request(const char* data) {
    return std::make_pair<std::string, std::string>({ data, data + 4 }, { data + 4 });
}


class peer_manager : public std::enable_shared_from_this<peer_manager> {
    static constexpr auto DEFAULT_KEEP_ALIVE = std::chrono::seconds(10);
    static constexpr auto DEFAULT_TIMEOUT    = std::chrono::seconds(30);

public:
    using address_type = net::address_v4;
    using peer_type    = net::address_v4;
    using time_type    = clocks::time_type;

    explicit peer_manager(net::io_context& ioc, std::shared_ptr<shared_state> state, bool debug = false)
            : m_socket(), m_ioc(ioc), m_state(std::move(state)), debug_mode(debug) {
        m_socket.bind(m_state->address());
        m_state->join(m_state->address());
    }

    explicit peer_manager(net::io_context& ioc, const std::vector<peer_type>& peers, std::shared_ptr<shared_state> state, bool debug = false)
            : peer_manager(ioc, std::move(state), debug) {
        for(const auto& peer : peers) {
            m_state->join(peer);
//            log<log_type::peers>(peer.to_string());
        }
    }

    void run() {
        std::thread([self = shared_from_this()](net::udp::socket s) {
            self->update(s);
        }, std::move(m_socket.clone())).detach();

        std::thread([self = shared_from_this()](net::udp::socket s) {
            self->broadcast(s);
        }, std::move(m_socket.clone())).detach();

        auto listen_thread = std::thread([self = shared_from_this()](net::udp::socket s) {
            self->listen(s);
        }, std::move(m_socket.clone()));
        listen_thread.join();

        m_state->halt();
    }

private:
    void broadcast(const net::udp::socket& sock) {
        while(m_state->is_running()) {
            if(m_ioc.has_outgoing()) {
                const auto message = m_ioc.pop_outgoing();
                multicast_snippet(sock, message);
            }
            std::this_thread::sleep_for(milliseconds(500));
        }
    }

    void update(const net::udp::socket& sock) {
        if(debug_mode)
            std::cerr << "Scheduling keepalive updates..." << std::endl;
        while(m_state->is_running()) {
            if(debug_mode)
                std::cerr << "Sending keepalive messages" << std::endl;
            multicast_update(sock);
            if(debug_mode)
                std::cerr << "Removing old peers" << std::endl;
            clean_peer_list();
            std::this_thread::sleep_for(DEFAULT_KEEP_ALIVE);
        }
    }

    void listen(const net::udp::socket& sock) {
        address_type sender;
        if(debug_mode)
            std::cerr << "Listening for messages..." << std::endl;
        while(true) {
            char data[2048] = {};
            sock.recv_from(net::buffer(data), &sender);
            auto [request, contents] = parse_request(data);
            if(debug_mode) std::cerr << "Got '" << request << "' request from " << sender.to_string() << ": " << contents << std::endl;
            if(request == "peer")
                on_peer(sender, contents);
            else if(request == "snip")
                on_snip(sender, contents);
            else if(request == "stop")
                break;
        }
    }

    void multicast_update(const net::udp::socket& sock)  {
        const std::string message = "peer" + sock.address().to_string();
        basic_multicast(sock, message);
//        for(const auto& [addr, time] : m_state->peers())
//            log<log_type::send_peers>(strings::join(" ", addr.to_string(), sock.address().to_string()));
    }

    void multicast_snippet(const net::udp::socket& sock, const std::string& message) {
        const std::string snippet = "snip" + std::to_string(m_state->timestamp()) + " " + message;
        m_state->increment_timestamp();
        basic_multicast(sock, snippet);
    }

    void on_peer(const address_type& sender, const std::string& content) {
        const auto& [host, port] = strings::split(content, ':');
        const peer_type new_peer = { host, static_cast<in_port_t>(std::stoul(port)) };
        update_peer(sender);
        update_peer(new_peer);
//        log<log_type::peers>(sender.to_string());
//        log<log_type::peers>(new_peer.to_string());
//        log<log_type::recv_peers>(sender.to_string() + " " + new_peer.to_string());
        if(debug_mode) std::cerr << "Handled peer request" << std::endl;
    }

    void on_snip(const address_type& sender, const std::string& content) {
        const auto message = strings::split(content, ' ');
        const auto timestamp = std::stoul(message.first);
        const auto snippet = message.second;
        if(sender != m_state->address())
            m_ioc.put_incoming(snippet);
        m_state->update(sender);
        m_state->update_timestamp(timestamp);
//        log<log_type::snippets>(std::to_string(m_state->timestamp()) + " " + snippet + " " + sender.to_string());
    }

    void clean_peer_list() {
        const auto before = m_state->peers().size();
        const auto current_time = clocks::get_current_time();
        const shared_state::peer_map current_peers = { m_state->peers() };
        for(const auto& [address, time] : current_peers) {
            const auto time_elapsed = current_time - time;
            if(time_elapsed > DEFAULT_TIMEOUT) {
                std::cerr << "Removed " << address << std::endl;
                remove_peer(address);
            }
        }
        if(debug_mode) std::cerr << "Removed " << before - m_state->peers().size() << " peers" << std::endl;
    }

    void basic_multicast(const net::udp::socket& sock, const std::string& message) const {
        if(debug_mode) std::cerr << "Sending '" << message << "' to " << m_state->peers().size() << " peers." << std::endl;
        for(const auto& [addr, time] : m_state->peers()) {
            if(addr == sock.address()) continue;
            sock.send_to(net::buffer(message), addr);
        }
    }

    void update_peer(const peer_type& peer) {
        m_state->update(peer);
    }

    void remove_peer(const peer_type& peer) {
        m_state->leave(peer);
    }

    net::io_context& m_ioc;

    std::shared_ptr<shared_state> m_state;
    net::udp::socket m_socket;

    const bool debug_mode;
};


//std::string assemble_report(const peer_manager& manager) {
//    std::stringstream report;
//
//    // Report all peers
//    const auto& peers_log = manager.log<log_type::peers>();
//    report << peers_log.size() << '\n';
//    std::for_each(peers_log.begin(), peers_log.end(), [&](const auto& peer){
//        report << peer.message << '\n';
//    });
//
//    // Report peers learned by sources
//
//    // Report peers acquired by 'peer' updates
//    const auto& recv_peers_log = manager.log<log_type::recv_peers>();
//    report << recv_peers_log.size() << '\n';
//    std::for_each(recv_peers_log.begin(), recv_peers_log.end(), [&](const auto& peer){
//        report << peer.message << ' ' << peer.date << '\n';
//    });
//
//    // Report sent 'peer' updates
//    const auto& send_peers_log = manager.log<log_type::send_peers>();
//    report << send_peers_log.size() << '\n';
//    std::for_each(send_peers_log.begin(), send_peers_log.end(), [&](const auto& peer){
//        report << peer.message << ' ' << peer.date << '\n';
//    });
//
//    // Report all snippets
//    const auto& snippet_log = manager.log<log_type::snippets>();
//    report << snippet_log.size() << '\n';
//    std::for_each(snippet_log.begin(), snippet_log.end(), [&](const auto& snippet) {
//        report << snippet.message << '\n';
//    });
//
//    return report.str();
//}

#endif //PEER_MANAGER_HPP
