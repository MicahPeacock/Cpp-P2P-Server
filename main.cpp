#include "net/udp.hpp"
#include "registry.hpp"
#include "peer_manager.hpp"

#include <iostream>

int main(int argc, const char* argv[]) {
    const std::string name = "P.E.A.C.O.C.K.";
    const net::address_v4 addr = { "136.159.5.22", 55921 };
    const net::address_v4 client_addr = net::address_v4(12000);

    registry::context ctx = { name };
    registry::run(client_addr, addr, ctx);

    io_context ioc;
    std::make_shared<peer_manager>(ioc, ctx.peers, std::make_shared<shared_state>(ctx.address))->run();

    registry::run(client_addr, addr, ctx);

    return 0;
}
