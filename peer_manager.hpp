#ifndef PEER_MANAGER_HPP
#define PEER_MANAGER_HPP

#include "net/buffer.hpp"
#include "net/udp.hpp"

#include "io_context.hpp"
#include "logger.hpp"
#include "shared_state.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std::chrono;

/**
 * Takes the data from an incoming message and splits the message between the request and its contents.
 * @param data the
 * @return a pair where the first element is the request, and the second element is the contents.
 */
auto parse_request(const char* data) {
    return std::make_pair<std::string, std::string>({ data, data + 4 }, { data + 4 });
}


/**
 * This class manages the lifetime of the peer to peer chat server. When run the manager will handle three threads:
 *     - An update thread which sends 'heartbeat' messages to inform the other peers the client is alive,
 *          as well as removes any inactive peers in the network.
 *     - A broadcast thread which multicasts any outgoing messages from the client
 *     - A listening thread which receives and handles any incoming message from other peers.
 */
class peer_manager : public std::enable_shared_from_this<peer_manager>, public logger {
    static constexpr auto DEFAULT_KEEP_ALIVE = std::chrono::seconds(5);
    static constexpr auto DEFAULT_TIMEOUT    = std::chrono::seconds(20);

public:
    using address_type = net::address_v4;
    using peer_type    = net::address_v4;
    using time_type    = clocks::time_type;

    explicit peer_manager(net::io_context& ioc, std::shared_ptr<shared_state> state, bool debug = false)
            : m_socket(), m_ioc(ioc), m_state(std::move(state)), debug_mode(debug) {
        m_socket.bind(m_state->address());
    }

    explicit peer_manager(net::io_context& ioc, const net::address_v4& src, const std::unordered_set<peer_type>& peers, std::shared_ptr<shared_state> state, bool debug = false)
            : peer_manager(ioc, std::move(state), debug) {
        for(const auto& peer : peers) {
            m_state->join(peer);
            log_peer(peer.to_string());
        }
        log_source(src.to_string(), peers);
    }

    /**
     * Starts the peer manager.
     * The lifetime of the manager depends on the listening thread and will halt once the listening thread finishes.
     */
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
    /**
     * Sends any outgoing messages from the snippet interface and broadcasts them to the other peers.
     * @param sock The UDP socket to send the message.
     */
    void broadcast(const net::udp::socket& sock) {
        while(m_state->is_running()) {
            if(m_ioc.has_outgoing()) {
                const auto message = m_ioc.pop_outgoing();
                multicast_snippet(sock, message);
            }
            std::this_thread::sleep_for(milliseconds(200));
        }
    }

    /**
     * Sends occasional 'heartbeat' messages to the other peers, and removes any inactive peers from the network.
     * @param sock The UDP socket to send the messages.
     */
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

    /**
     * Listens for incoming requests from other peers and handles requests. The process will close once the "stop"
     * command has been received.
     * @param sock The UDP socket to send/receive the the messages.
     */
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
                on_peer(sender, strings::trim(contents));
            else if(request == "snip")
                on_snip(sender, strings::trim(contents));
            else if(request == "stop")
                break;
        }
    }

    /**
     * Sends a 'heartbeat' message to all active peers.
     * @param sock The UDP socket to send the message.
     */
    void multicast_update(const net::udp::socket& sock)  {
        const std::string message = "peer" + sock.address().to_string();
        basic_multicast(sock, message);
        for(const auto& [addr, time] : m_state->peers())
            log_sent_peer(addr.to_string(), sock.address().to_string());
    }

    /**
     * Sends a snippet message to all active peers.
     * @param sock The UDP socket to send the message.
     * @param message The snippet message to send.
     */
    void multicast_snippet(const net::udp::socket& sock, const std::string& message) {
        m_state->increment_timestamp();
        const std::string snippet = "snip" + std::to_string(m_state->timestamp()) + " " + message;
        basic_multicast(sock, snippet);
    }

    /**
     * Request handler to handle 'peer' requests.
     * If an invalid net address has been received, the peer update will be ignored.
     * @param sender
     * @param content
     */
    void on_peer(const address_type& sender, const std::string& content) {
        const auto& [host, port] = strings::split(content, ':');
        try {
            const peer_type new_peer = { host, static_cast<in_port_t>(std::stoul(port)) };
            update_peer(sender);
            update_peer(new_peer);
            log_peer(sender.to_string());
            log_peer(new_peer.to_string());
            log_recv_peer(sender.to_string(), new_peer.to_string());
            if(debug_mode) std::cerr << "Handled peer request" << std::endl;
        } catch(net::address_error& err) {
            std::cerr << err.what() << std::endl;
        }
    }

    /**
     * Request handler to handle 'snip' requests.
     * To handle Lamport ordering, the timestamp of the manager will be updated to the maximum between
     * the current timestamp and the timestamp of the message.
     * @param sender The address of the sender of the snippet message.
     * @param content The contents of the snippet message.
     */
    void on_snip(const address_type& sender, const std::string& content) {
        const auto message = strings::split(content, ' ');
        const auto timestamp = std::stoul(message.first);
        const auto snippet = message.second;
        m_state->update(sender);
        m_state->update_timestamp(timestamp);
        m_ioc.put_incoming(sender, snippet, m_state->timestamp());
        log_snippet(m_state->timestamp(), snippet, sender.to_string());
    }

    /**
     * Removes inactive peers from the active peers list.
     * This is done by a timeout. When the last updated time of the peer exceeds the default timeout (20 seconds),
     * the peer is treated as inactive and removed from the network.
     */
    void clean_peer_list() {
        const auto before = m_state->peers().size();
        const auto current_time = clocks::get_current_time();
        const shared_state::peer_map current_peers = { m_state->peers() };
        for(const auto& [address, time] : current_peers) {
            const auto time_elapsed = current_time - time;
            if(time_elapsed > DEFAULT_TIMEOUT)
                remove_peer(address);
        }
        if(debug_mode) std::cerr << "Removed " << before - m_state->peers().size() << " peers" << std::endl;
    }

    /**
     * Sends a message to every peer in the network.
     * @param sock The UDP socket to send the message.
     * @param message The message to send.
     */
    void basic_multicast(const net::udp::socket& sock, const std::string& message) const {
        if(debug_mode) std::cerr << "Sending '" << message << "' to " << m_state->peers().size() << " peers." << std::endl;
        for(const auto& [addr, time] : m_state->peers()) {
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


/**
 * Takes the current state of a peer manager and generates a runtime report of the peer manager.
 * @param manager The peer manager to print.
 * @return a string representing the peer server report.
 */
std::string assemble_report(const peer_manager& manager) {
    std::stringstream report;

    // Report all peers
    const auto& peers_log = manager.peer_log();
    report << peers_log.size() << '\n';
    for(const auto& peer : peers_log)
        report << peer << '\n';

    // Report peers learned by sources
    const auto& source_log = manager.source_log();
    report << source_log.size() << '\n';
    for(const auto& [src, entry] : source_log) {
        report << src << '\n' << entry.date << '\n';
        report << entry.peers.size() << '\n';
        for(const auto& peer : entry.peers)
            report << peer.to_string() << '\n';
    }

    // Report peers acquired by 'peer' updates
    const auto& recv_peers_log = manager.recv_peers_log();
    report << recv_peers_log.size() << '\n';
    for(const auto& peer : recv_peers_log)
        report << peer.to << ' ' << peer.from << ' ' << peer.date << '\n';

    // Report sent 'peer' updates
    const auto& send_peers_log = manager.sent_peers_log();
    report << send_peers_log.size() << '\n';
    for(const auto& peer : send_peers_log)
        report << peer.to << ' ' << peer.from << ' ' << peer.date << '\n';

    // Report all snippets
    const auto& snippet_log = manager.snippet_log();
    report << snippet_log.size() << '\n';
    for(const auto& snippet : snippet_log)
        report << snippet.timestamp << ' ' << snippet.message << ' ' << snippet.sender << '\n';

    return report.str();
}

#endif //PEER_MANAGER_HPP
