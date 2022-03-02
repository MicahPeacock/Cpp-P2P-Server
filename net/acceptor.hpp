#ifndef ACCEPTOR_HPP
#define ACCEPTOR_HPP

#include "socket_address.hpp"
#include "stream_socket.hpp"

namespace net {

/**
 * Class for creating a streaming server.
 * The call to accept creates and returns a @em stream_socket
 * instance which can be used for the actual communications.
 *
 * This class can be customized to use any socket and address classes. By default,
 * the address class will be the one already specified in the socket class.
 * This class is best used when simplified with type aliasing.
 *
 *     using tcp_acceptor = net::acceptor<net::tcp::socket>;
 *
 * @tparam StreamSocket The class of stream socket the acceptor will create.
 * @tparam AddrType The class of the address representation.
 */
template<typename StreamSocket, typename AddrType = typename StreamSocket::address_t>
class acceptor : public socket<AddrType> {
    using base_t = socket<AddrType>;

public:
    using stream_socket_t = StreamSocket;
    using address_t = AddrType;

    // Non-copyable
    acceptor(const acceptor&) = delete;
    acceptor& operator=(const acceptor&) = delete;

    /**
     * Creates an unconnected acceptor.
     */
    acceptor() = default;

    /**
     *
     * @param handle
     */
    explicit acceptor(socket_t handle)
            : base_t(handle) {}

    /**
     *
     * @param addr The TCP address on which to listen.
     * @param queue_size The listener queue size.
     */
    explicit acceptor(const address_t& addr, int queue_size = DEFAULT_QUEUE_SIZE) {
        open(addr, queue_size);
    }

    /**
    * Move constructor.
    * Creates an acceptor by moving the other acceptor to this one.
    * @param acc Another acceptor
    */
    acceptor(acceptor&& acc) noexcept
            : base_t(std::move(acc)) {}

    /**
     * Move assignment.
     * @param rhs The other socket to move into this one.
     * @return A reference to this object
     */
    acceptor& operator=(acceptor&& rhs) noexcept {
        base_t::operator=(std::move(rhs));
        return *this;
    }

    /**
     *
     * @return An open, but unbound acceptor socket.
     */
    static acceptor create() {
        acceptor acc(create_handle(address_t::ADDRESS_FAMILY));
        if(!acc) acc.clear(base_t::get_last_error());
        return acc;
    }

    /**
     *
     * @param queue_size The listener queue size.
     * @return
     */
    bool listen(int queue_size = DEFAULT_QUEUE_SIZE) {
        return base_t::check_return_bool(::listen(base_t::handle(), queue_size));
    }

    /**
     *
     * @param addr
     * @param queue_size The listener queue size.
     * @param reuse_sock
     * @return
     */
    template<address_family Family>
    bool open(const socket_address<Family>& addr, int queue_size = DEFAULT_QUEUE_SIZE, bool reuse_sock = true) {
        if(base_t::is_open())
            return true;
        const sa_family_t domain = addr.family();
        const socket_t handle = create_handle(domain);
        if(!base_t::check_socket_bool(handle))
            return false;
        base_t::reset(handle);
        if(reuse_sock && domain == AF_INET) {
            int reuse = 1;
            if(!base_t::set_option(SOL_SOCKET, SO_REUSEPORT, reuse))
                return base_t::close_on_error();
        }
        if(!bind(addr) || !listen(queue_size))
            return base_t::close_on_error();
        return true;
    }

    /**
     *
     * @param port
     * @param queue_size The listener queue size.
     * @return
     */
    bool open(in_port_t port, int queue_size = DEFAULT_QUEUE_SIZE) {
        return open(address_t(port), queue_size);
    }

    /**
     * Accepts an incoming connection and gets the address of the client.
     * @param client_addr Pointer to the variable which will get the client address
     *                    information when it successfully connects.
     * @return stream_socket to the remote client.
     */
    stream_socket_t accept(address_t* client_addr = nullptr) {
        sockaddr* p = client_addr ? client_addr->sockaddr_ptr() : nullptr;
        socklen_t len = client_addr ? client_addr->size() : 0;
        socket_t s = base_t::check_socket(::accept(base_t::handle(), p, client_addr ? &len : nullptr));
        return stream_socket_t(s);
    }

protected:
    static constexpr int DEFAULT_QUEUE_SIZE = 5;

    /**
     *
     * @param domain
     * @return
     */
    static socket_t create_handle(int domain) {
        return stream_socket_t::create_handle(domain);
    }
};

} // net

#endif // ACCEPTOR_HPP
