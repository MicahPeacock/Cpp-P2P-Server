#ifndef REGISTRY_HPP
#define REGISTRY_HPP

#include "net/socket_address.hpp"
#include "net/tcp.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <unordered_set>
#include <utility>


namespace fs = std::filesystem;

namespace registry {

using address_type = net::address_v4;
using peer_type    = net::address_v4;
using socket_type  = net::tcp::connector;

/**
 *
 */
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

/**
 *
 */
struct context {
    std::string name;
    std::string filepath = ".";
    address_type address;
    std::string report;
    std::unordered_set<peer_type> peers;
};

/**
 * Prints any errors
 * @param sock
 */
void handle_error(const socket_type& sock) noexcept {
    if(sock.last_error() != 0)
        std::cerr << sock.last_error_str() << std::endl;
}

/**
 * Given a TCP socket and a max number of bytes, will receive the data across the socket, and print any errors encountered.
 * @param sock A TCP socket to read from.
 * @param n Maximum number of bytes to read from the socket (Max 128);
 * @return a
 */
std::string read(const socket_type& sock, size_t n = 128) noexcept {
    char payload[128] = {};
    sock.read(net::buffer(payload, n));
    handle_error(sock);
    return { payload };
}

/**
 * Given a TCP socket and a string of data, will send the data across the socket, and print any errors encountered.
 * @param sock A TCP socket to write to.
 * @param payload the data to send across the given socket.
 */
void write(const socket_type& sock, const std::string& payload) noexcept {
    sock.write(net::buffer(payload + "\n"));
    handle_error(sock);
}

/**
 * Given a
 * @param path Filepath of the directory to iterate.
 * @return A vector of relative filepaths to source code files.
 */
std::vector<std::string> get_source_files(const std::string& path) {
    using file_iterator = fs::recursive_directory_iterator;
    std::vector<std::string> ret;
    for(const auto& file : file_iterator(path)) {
        if(file.path().extension() == ".cpp" || file.path().extension() == ".hpp")
            ret.push_back(file.path().relative_path());
    }
    return ret;
}

/**
 * Reads the entire contents of the file into a string. Returns an empty string if opening the file is unsuccessful.
 * @param filepath Relative filepath of the file to open.
 * @return The contents of the file as a string.
 */
std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    if(!file.is_open()) return "";
    return { std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>() };
}

/**
 * Registry request handlers
 */
using request_handler_t = std::function<void(socket_type&, context&)>;
static const std::unordered_map<request, request_handler_t> request_handlers = {
        {request::name,     [](socket_type& sock, context& ctx) {
            registry::write(sock, ctx.name);
        }},
        {request::location, [](socket_type& sock, context& ctx){
            registry::write(sock, sock.address().to_string());
        }},
        {request::code,     [](socket_type& sock, context& ctx){
            registry::write(sock, "cpp");
            const auto files = get_source_files(ctx.filepath);
            for(const auto& filename : files)
                registry::write(sock, read_file(filename));
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
                if(saddr != "null")
                    ctx.peers.insert(addr);
            }
            if(strings::ends_with(data, "close\n"))
                sock.close();
        }},
        {request::close,    [](socket_type& sock, context& ctx) {
            sock.close();
        }}
};


/**
 * Creates a connection to the specified registry. This function will continue to receive requests from the registry
 *      until it receives the "close" command.
 * @param client_addr The address to bind the client to.
 * @param registry_addr The address of the registry to connect to.
 * @param ctx The registry context which contains information saved from the registry.
 */
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