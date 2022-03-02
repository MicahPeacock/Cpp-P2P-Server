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

    static constexpr bool DEBUG = true;

public:
    using address_type = net::address_v4;
    using peer_type    = net::address_v4;
    using time_type    = clocks::time_type;

    explicit peer_manager(io_context& ioc, std::shared_ptr<shared_state> state, bool debug = false)
            : m_socket(), m_ioc(ioc), m_state(std::move(state)), debug_mode(debug) {
        m_socket.bind(m_state->address());
        m_state->join(m_state->address());
    }

    explicit peer_manager(io_context& ioc, const std::vector<peer_type>& peers, std::shared_ptr<shared_state> state, bool debug = false)
            : peer_manager(ioc, std::move(state), debug) {
        for(const auto& peer : peers)
            m_state->join(peer);
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
            if(!m_ioc.outgoing().empty()) {
                const auto message = m_ioc.pop_outgoing();
                multicast_snippet(sock, message);
            }
            std::this_thread::sleep_for(seconds(1));
        }
    }

    void update(const net::udp::socket& sock) {
        if(debug_mode) std::cout << "Scheduling keepalive updates..." << std::endl;
        while(m_state->is_running()) {
            if(debug_mode) std::cout << "Sending keepalive messages" << std::endl;
            multicast_update(sock);
            if(debug_mode) std::cout << "Removing old peers" << std::endl;
            clean_peer_list();
            std::this_thread::sleep_for(seconds(10));
        }
    }

    void listen(const net::udp::socket& sock) {
        address_type sender;
        if(debug_mode) {
            std::cout << "Listening for messages..." << std::endl;
        }
        while(true) {
            char data[128] = {};
            sock.recv_from(net::buffer(data), &sender);
            auto [request, contents] = parse_request(data);
            if(debug_mode) std::cout << "Got '" << request << "' request from " << sender.to_string() << ": " << contents << std::endl;
            if(request == "peer")
                std::thread([self = shared_from_this()](address_type peer, const std::string& message){
                    self->on_peer(peer, message);
                }, sender, std::move(contents)).detach();
            else if(request == "snip")
                std::thread([self = shared_from_this()](address_type peer, const std::string& message) {
                    self->on_snip(peer, message);
                }, sender, std::move(contents)).detach();
            else if(request == "stop")
                break;
        }
    }

    void multicast_update(const net::udp::socket& sock) const {
        const std::string message = "peer" + sock.address().to_string();
        basic_multicast(sock, message);
//        for(const auto& [addr, time] : m_state->peers())
//            log<log_type::send_peers>(strings::join(" ", addr.to_string(), sock.address().to_string()));
        // TODO: Record in logs
        // TODO: Record in debug
    }

    void multicast_snippet(const net::udp::socket& sock, const std::string& message) {
        const std::string msg = "snip" + std::to_string(m_state->timestamp()) + " " + message;
        m_state->increment_timestamp();
        basic_multicast(sock, msg);
    }

    void on_peer(const address_type& sender, const std::string& content) {
        const auto& [host, port] = strings::split(content, ':');
        const peer_type new_peer = { host, static_cast<in_port_t>(std::stoul(port)) };
        update_peer(sender);
        update_peer(new_peer);
        // TODO: Record in logs
        // TODO: Record in debug
        if(debug_mode) std::cout << "Handled peer request" << std::endl;
    }

    void on_snip(const address_type& sender, const std::string& content) {
        const auto message = strings::split(content, ' ');
        const auto timestamp = std::stoul(message.first);
        const auto snippet = message.second;
        if(sender != m_state->address())
            m_ioc.put_incoming(snippet);
        m_state->update(sender);
        m_state->update_timestamp(timestamp);
    }

    void clean_peer_list() {
        const auto before = m_state->peers().size();
        const auto current_time = clocks::get_current_time();
        for(const auto& [address, time] : m_state->peers()) {
            const auto time_elapsed = current_time - time;
            if(time_elapsed > DEFAULT_TIMEOUT) {
                std::cout << "Removed " << address << std::endl;
                remove_peer(address);
            }
        }
        if(debug_mode) std::cout << "Removed " << before - m_state->peers().size() << " peers" << std::endl;
    }

    void basic_multicast(const net::udp::socket& sock, const std::string& message) const {
        if(debug_mode) std::cout << "Sending '" << message << "' to " << m_state->peers().size() << " peers." << std::endl;
        for(const auto& [addr, time] : m_state->peers())
            sock.send_to(net::buffer(message), addr);
    }

    void update_peer(const peer_type& peer) {
        m_state->update(peer);
    }

    void remove_peer(const peer_type& peer) {
        m_state->leave(peer);
    }

    io_context& m_ioc;

    std::shared_ptr<logger> m_logger;
    std::shared_ptr<shared_state> m_state;
    net::udp::socket m_socket;

    const bool debug_mode;
};


//std::string assemble_report(const peer_manager& manager) {
//    std::stringstream report;
//    // Report all peers
//    // Report peers learned by sources
//
//    // Report peers acquired by 'peer' updates
//    const auto& recv_peers_log = manager.log<log_type::recv_peers>();
//    report << recv_peers_log.size() << '\n';
//    for(const auto& recv_peer: recv_peers_log)
//        report << recv_peer.message << '\n';
//
//    // Report sent 'peer' updates
//    const auto& send_peers_log = manager.log<log_type::send_peers>();
//    report << send_peers_log.size() << '\n';
//    for(const auto& send_peer: send_peers_log)
//        report << send_peer.message << '\n';
//
//    // Report all snippets
//    const auto& snippet_log = manager.log<log_type::snippets>();
//    report << snippet_log.size() << '\n';
//    for(const auto& snippet: snippet_log)
//        report << snippet.message << '\n';
//
//    return report.str();
//}

#endif //PEER_MANAGER_HPP
