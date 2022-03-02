#ifndef REGISTRY_HPP
#define REGISTRY_HPP

#include <filesystem>
#include <functional>
#include <utility>
#include <fstream>
#include "net/tcp.hpp"
#include "net/socket_address.hpp"

namespace fs = std::filesystem;

namespace registry {

using address_type = net::address_v4;
using peer_type    = net::address_v4;
using socket_type  = net::tcp::connector;

enum class request {
    empty,
    name,
    code,
    location,
    report,
    peers,
    close,
    invalid
};

inline request to_request(const std::string& str) {
    if(str.empty()) return request::empty;
    if(strings::contains(str, "get team name")) return request::name;
    if(strings::contains(str,      "get code")) return request::code;
    if(strings::contains(str,  "get location")) return request::location;
    if(strings::contains(str,    "get report")) return request::report;
    if(strings::contains(str, "receive peers")) return request::peers;
    if(strings::contains(str,         "close")) return request::close;
    return request::invalid;
}


struct context {
    std::string name;
    address_type address;
    std::string report;
    std::vector<peer_type> peers;
};


void handle_error(const socket_type& sock) noexcept {
    if(sock.last_error() != 0)
        std::cerr << sock.last_error_str() << std::endl;
}

std::string read(const socket_type& sock, size_t n = 128) noexcept {
    char payload[128] = {};
    sock.read(net::buffer(payload, n));
    handle_error(sock);
    return { payload };
}

void write(const socket_type& sock, const std::string& payload) noexcept {
    sock.write(net::buffer(payload + "\n"));
    handle_error(sock);
}


using request_handler_t = std::function<void(socket_type&, context&)>;
static const std::unordered_map<request, request_handler_t> request_handlers = {
        {request::name,     [](socket_type& sock, context& ctx) {
            registry::write(sock, ctx.name);
        }},
        {request::location, [](socket_type& sock, context& ctx){
            registry::write(sock, sock.address().to_string());
        }},
        {request::code,     [](socket_type& sock, context& ctx){
            registry::write(sock, ".cpp");
            registry::write(sock, "...");
        }},
        {request::report,   [](socket_type& sock, context& ctx){
            registry::write(sock, ctx.report);
        }},
        {request::peers,    [](socket_type& sock, context& ctx){
            const auto data = registry::read(sock);
            std::stringstream ss(data);
            std::string line;
            std::getline(ss, line);
            const size_t num_peers = std::stoul(line);
            for(size_t i = 0; i < num_peers; i++) {
                std::getline(ss, line);
                const auto& [saddr, port] = strings::split(line, ':');
                const address_type addr = { saddr, static_cast<in_port_t>(std::stoul(port)) };
                if(saddr != "null" && addr != sock.address())
                    ctx.peers.push_back(addr);
            }
            if(strings::ends_with(data, "close\n"))
                sock.close();
        }},
        {request::close,    [](socket_type& sock, context& ctx) {
            sock.close();
        }}
};


void run(const address_type& client_addr, const address_type& registry_addr, context& ctx) {
    socket_type registry(client_addr, registry_addr);
    if(!ctx.address.is_set())
        ctx.address = registry.address();
    while(registry.is_connected()) {
        const auto req = to_request(read(registry, 14));
        request_handlers.at(req)(registry, ctx);
    }
}

} // registry

#endif // REGISTRY_HPP