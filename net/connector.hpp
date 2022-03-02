#ifndef CONNECTOR_HPP
#define CONNECTOR_HPP

#include "socket_address.hpp"
#include "stream_socket.hpp"

namespace net {

/**
 *
 * @tparam StreamSocket
 * @tparam AddrType
 */
template<typename StreamSocket, typename AddrType = typename StreamSocket::address_t>
class connector : public stream_socket<AddrType> {
    using base_t = stream_socket<AddrType>;

public:
    using stream_socket_t = StreamSocket;
    using address_t = AddrType;

    // Non-copyable
    connector(const connector&) = delete;
    connector& operator=(const connector&) = delete;

    /**
     * Creates an unconnected connector.
     */
    connector() = default;

    /**
     *
     * @param addr
     */
    explicit connector(const address_t& addr) {
        connector::connect(addr);
    }

    explicit connector(const address_t& client_addr, const address_t& addr) {
        connector::connect(client_addr, addr);
    }

    /**
     *
     * @tparam Family
     * @param addr
     */
    template<address_family Family>
    explicit connector(const socket_address<Family>& addr) {
        connector::connect(addr);
    }

    /**
     *
     * @param conn
     */
    connector(connector&& conn) noexcept
            : base_t(std::move(conn)) {}

    /**
     *
     * @param rhs
     * @return
     */
    connector& operator=(connector&& rhs) noexcept {
        base_t::operator=(std::move(rhs));
        return *this;
    }

    /**
     *
     * @return
     */
    [[nodiscard]] bool is_connected() const {
        return stream_socket_t::is_open();
    }

    /**
     *
     * @param addr
     * @return
     */
    template<address_family Family>
    bool connect(const socket_address<Family>& addr) {
        sa_family_t domain = addr.family();
        socket_t h = stream_socket_t::create_handle(domain);
        if(!stream_socket_t::check_return_bool(h))
            return false;
        stream_socket_t::reset(h);
        if(!stream_socket_t::check_return_bool(::connect(stream_socket_t::handle(), addr.sockaddr_ptr(), addr.size())))
            return stream_socket_t::close_on_error();
        return true;
    }

    template<address_family Family>
    bool connect(const socket_address<Family>& client_addr, const socket_address<Family>& addr) {
        sa_family_t domain = addr.family();
        socket_t h = stream_socket_t::create_handle(domain);
        if(!stream_socket_t::check_return_bool(h))
            return false;
        stream_socket_t::reset(h);
        stream_socket_t::bind(client_addr);
        if(!stream_socket_t::check_return_bool(::connect(stream_socket_t::handle(), addr.sockaddr_ptr(), addr.size())))
            return stream_socket_t::close_on_error();
        return true;
    }
};

} // net

#endif // CONNECTOR_HPP
