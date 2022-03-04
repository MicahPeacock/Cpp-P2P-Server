#include "net/socket_address.hpp"
#include "registry.hpp"
#include "peer_manager.hpp"
#include "snippet_manager.hpp"

#include <iostream>


int main(int argc, const char* argv[]) {
    if(argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <team name> <port>";
        return EXIT_FAILURE;
    }
    const std::string name = argv[1];
    const size_t port = std::stoul(argv[2]);
    const net::address_v4 addr = { "136.159.5.22", 55921 };

    registry::context ctx = { name };
    std::cout << "Getting initial peers..." << std::endl;
    registry::run(net::address_v4(port), addr, ctx);

    net::io_context ioc;
    const auto snippets = std::make_shared<snippet_manager>(ioc);
    const auto manager  = std::make_shared<peer_manager>(ioc, addr, ctx.peers, std::make_shared<shared_state>(ctx.address));
    snippets->run();
    manager->run();     // This method is blocking, and will run once the peer manager receives 'stop'
    snippets->close();

    std::cout << "Sending report..." << std::endl;
    ctx.report = assemble_report(*manager);
    registry::run(net::address_v4(port), addr, ctx);
    return 0;
}
