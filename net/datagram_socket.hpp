#ifndef DATAGRAM_SOCKET_HPP
#define DATAGRAM_SOCKET_HPP

#include "buffer.hpp"
#include "socket.hpp"

namespace net {

/**
 *
 * @tparam AddrType
 */
template<typename AddrType>
class datagram_socket : public socket<AddrType> {
    using base_t = socket<AddrType>;

public:
    static constexpr int COMM_TYPE = SOCK_DGRAM;
    static constexpr sa_family_t ADDRESS_FAMILY = AddrType::ADDRESS_FAMILY;
    using address_t = AddrType;

    // Non-copyable
    datagram_socket(const datagram_socket&) = delete;
    datagram_socket& operator=(const datagram_socket&) = delete;

    /**
     *
     */
    datagram_socket()
            : base_t(create_handle(ADDRESS_FAMILY)) {}

    /**
     *
     * @param handle
     */
    explicit datagram_socket(socket_t handle)
            : base_t(handle) {}

    /**
     *
     * @param addr
     */
    explicit datagram_socket(const address_t& addr) {
        auto domain = addr.family();
        socket_t h = create_handle(domain);
        if(base_t::check_socket_bool(h)) {
            base_t::reset(h);
            bind(addr);
        }
    }

    /**
     *
     * @param sock
     */
    datagram_socket(datagram_socket&& sock) noexcept
            : base_t(std::move(sock)) {}

    /**
     *
     * @param rhs
     * @return
     */
    datagram_socket& operator=(datagram_socket&& rhs) noexcept {
        base_t::operator=(std::move(rhs));
        return *this;
    }

    /**
     *
     * @param protocol
     * @return
     */
    static std::pair<datagram_socket, datagram_socket> pair(int protocol = 0) {
        auto pr = base_t::pair(ADDRESS_FAMILY, COMM_TYPE, protocol);
        return std::make_pair<datagram_socket, datagram_socket>(
                datagram_socket(pr.first.release()),
                datagram_socket(pr.second.release()));
    }

    datagram_socket clone() const {
        auto handle = base_t::clone().release();
        return datagram_socket(handle);
    }

    /**
     *
     * @param addr
     * @return
     */
    bool connect(const address_t& addr) {
        return base_t::check_return_bool(::connect(base_t::handle(), addr.sockaddr_ptr(), addr.size()));
    }

    /**
     *
     * @tparam Family
     * @param addr
     * @return
     */
    template<address_family Family>
    bool connect(const socket_address<Family>& addr) {
        return base_t::check_return_bool(::connect(base_t::handle(), addr.sockaddr_ptr(), addr.size()));
    }

    /**
     *
     * @tparam Family
     * @param b
     * @param flags
     * @param dst_addr
     * @return
     */
    template<address_family Family>
    ssize_t send_to(const const_buffer& payload, int flags, const socket_address<Family>& dst_addr) const noexcept {
        return base_t::check_return(::sendto(base_t::handle(), payload.data(), payload.size(), flags, dst_addr.sockaddr_ptr(), dst_addr.size()));
    }

    /**
     *
     * @tparam Family
     * @param b
     * @param dst_addr
     * @return
     */
    template<address_family Family>
    ssize_t send_to(const const_buffer& payload, const socket_address<Family>& dst_addr) const noexcept {
        return send_to(payload, 0, dst_addr);
    }

    /**
     *
     * @param s
     * @param flags
     * @return
     */
    ssize_t send(const const_buffer& payload, int flags = 0) const noexcept {
        return base_t::check_return(::send(base_t::handle(), payload.data(), payload.size(), flags));
    }

    /**
     *
     * @tparam Family
     * @param payload
     * @param flags
     * @param src_addr
     * @return
     */
    template<address_family Family>
    ssize_t recv_from(const mutable_buffer& payload, int flags, socket_address<Family>* src_addr = nullptr) const noexcept {
        sockaddr* p = src_addr ? src_addr->sockaddr_ptr() : nullptr;
        socklen_t len = src_addr ? src_addr->size() : 0;
        return base_t::check_return(::recvfrom(base_t::handle(), payload.data(), payload.size(), flags, p, &len));
    }

    /**
     *
     * @tparam Family
     * @param payload
     * @param src_addr
     * @return
     */
    template<address_family Family>
    ssize_t recv_from(const mutable_buffer& payload, socket_address<Family>* src_addr = nullptr) const noexcept {
        return recv_from(payload, 0, src_addr);
    }

    /**
     *
     * @param payload
     * @param flags
     * @return
     */
    [[nodiscard]] ssize_t recv(const mutable_buffer& payload, int flags = 0) const noexcept {
        return base_t::check_return(::recv(base_t::handle(), payload.data(), payload.size(), flags));
    }

protected:
    /**
     *
     * @param domain
     * @return
     */
    static socket_t create_handle(int domain) {
        return socket_t(::socket(domain, COMM_TYPE, 0));
    }
};

} // net

#endif // DATAGRAM_SOCKET_HPP
