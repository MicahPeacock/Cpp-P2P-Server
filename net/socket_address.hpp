#ifndef SOCKET_ADDRESS_HPP
#define SOCKET_ADDRESS_HPP

#include "exception.hpp"
#include "platform.hpp"

#include "../utils.hpp"

#include <cstring>


namespace net {

/**
 *
 */
enum class address_family {
    any, ipv4, ipv6
};

/**
 *
 * @tparam Family
 */
template<address_family Family>
class socket_address {
    [[nodiscard]] socklen_t size() const noexcept;
    [[nodiscard]]       sockaddr* sockaddr_ptr()       noexcept;
    [[nodiscard]] const sockaddr* sockaddr_ptr() const noexcept;
    [[nodiscard]] sa_family_t family() const noexcept;
    [[nodiscard]] std::string to_string() const noexcept;
};


/**
 *
 */
template<>
class socket_address<address_family::any> {
    static constexpr socklen_t MAX_SIZE = sizeof(sockaddr_storage);

public:
    using storage_t = sockaddr_storage;
    static constexpr sa_family_t ADDRESS_FAMILY = AF_UNSPEC;

    /**
     *
     */
    socket_address() = default;

    /**
     *
     * @param addr
     * @param n
     */
    socket_address(const sockaddr* addr, socklen_t n) {
        if(n > MAX_SIZE)
            throw socket_exception("Address length out of range");
        std::memcpy(&m_addr, addr, m_size = n);
    }

    /**
     *
     * @param addr
     * @param n
     */
    socket_address(const storage_t& addr, socklen_t n) {
        if(n > MAX_SIZE)
            throw socket_exception("Address length out of range");
        std::memcpy(&m_addr, &addr, m_size = n);
    }

    /**
     *
     * @tparam Family
     * @param addr
     */
    template<address_family Family>
    explicit socket_address(const socket_address<Family>& addr)
            : socket_address(addr.sockaddr_ptr(), addr.size()){}

    /**
     *
     * @return
     */
    [[nodiscard]] socklen_t size() const noexcept {
        return m_size;
    }

    /**
     *
     * @return
     */
    [[nodiscard]] sockaddr* sockaddr_ptr() noexcept {
        return reinterpret_cast<sockaddr*>(&m_addr);
    }

    /**
     *
     * @return
     */
    [[nodiscard]] const sockaddr* sockaddr_ptr() const noexcept {
        return reinterpret_cast<const sockaddr*>(&m_addr);
    }

    /**
     *
     * @return
     */
    [[nodiscard]] std::string to_string() const noexcept {
        return "<unknown>:<unknown>";
    }

private:
    storage_t m_addr = {};
    socklen_t m_size = MAX_SIZE;
};


/**
 *
 */
template<>
class socket_address<address_family::ipv4> {
    static constexpr size_t SIZE = sizeof(sockaddr_in);

public:
    using storage_t = sockaddr_in;
    static constexpr sa_family_t ADDRESS_FAMILY = AF_INET;

    /**
     * Constructs an empty socket_address
     */
    socket_address() = default;

    /**
     *
     * @param port The port number in native byte order.
     */
    explicit socket_address(in_port_t port)
            : m_addr(create(in_addr_t(INADDR_ANY), port)) {}

    /**
     *
     * @param addr
     * @param port
     */
    socket_address(in_addr_t addr, in_port_t port)
            : m_addr(create(addr, port)) {}

    /**
     *
     * @param addr
     * @param port
     */
    socket_address(const std::string& saddr, in_port_t port)
            : m_addr(create(saddr, port)) {}

    /**
     *
     * @param addr
     */
    explicit socket_address(const sockaddr& addr) {
        std::memcpy(&m_addr, &addr, SIZE);
    }

    /**
     *
     * @param addr
     */
    template<address_family Family>
    explicit socket_address(const socket_address<Family>& addr) {
        std::memcpy(&m_addr, addr.sockaddr_ptr(), SIZE);
    }

    /**
     *
     * @param addr
     */
    explicit socket_address(const sockaddr_in& addr)
            : m_addr(addr) {}

    /**
     *
     * @param addr
     */
    socket_address(const socket_address& addr) = default;

    /**
     *
     * @param saddr
     * @return
     */
    static in_addr_t resolve_name(const std::string& saddr) {
        addrinfo* response, hints = addrinfo{};
        hints.ai_family = ADDRESS_FAMILY;
        hints.ai_socktype = SOCK_STREAM;

        if(int ec = ::getaddrinfo(saddr.c_str(), nullptr, &hints, &response) != 0)
            throw address_error(ec, saddr);
        const auto ipv4 = reinterpret_cast<storage_t*>(response->ai_addr);
        const auto addr = ipv4->sin_addr.s_addr;
        freeaddrinfo(response);
        return addr;
    }

    /**
     *
     * @param addr
     * @param port
     * @return
     */
    static storage_t create(in_addr_t addr, in_port_t port) {
        storage_t ret = {};
        ret.sin_family = ADDRESS_FAMILY;
        ret.sin_addr.s_addr = addr;
        ret.sin_port = htons(port);
        return ret;
    }

    /**
     *
     * @param addr
     * @param port
     * @return
     */
    static storage_t create(const std::string& saddr, in_port_t port) {
        storage_t ret = {};
        ret.sin_family = ADDRESS_FAMILY;
        ret.sin_addr.s_addr = resolve_name(saddr);
        ret.sin_port = htons(port);
        return ret;
    }

    /**
     *
     * @return
     */
    [[nodiscard]] bool is_set() const noexcept {
        static const auto EMPTY_ADDR = storage_t{};
        return std::memcmp(&m_addr, &EMPTY_ADDR, SIZE) != 0;
    }

    /**
     *
     * @return
     */
    [[nodiscard]] in_addr_t address() const noexcept {
        return ntohl(m_addr.sin_addr.s_addr);
    }

    /**
     *
     * @return
     */
    [[nodiscard]] in_port_t port() const noexcept {
        return ntohs(m_addr.sin_port);
    }

    /**
     *
     * @return
     */
    [[nodiscard]] socklen_t size() const noexcept {
        return socklen_t(SIZE);
    }

    /**
     *
     * @return
     */
    [[nodiscard]] sockaddr* sockaddr_ptr() noexcept {
        return reinterpret_cast<sockaddr*>(&m_addr);
    }

    /**
     *
     * @return
     */
    [[nodiscard]] const sockaddr* sockaddr_ptr() const noexcept {
        return reinterpret_cast<const sockaddr*>(&m_addr);
    }

    /**
     *
     * @return
     */
    [[nodiscard]] storage_t* sockaddr_in_ptr() noexcept {
        return static_cast<storage_t*>(&m_addr);
    }

    /**
     *
     * @return
     */
    [[nodiscard]] const storage_t* sockaddr_in_ptr() const noexcept {
        return static_cast<const storage_t*>(&m_addr);
    }

    /**
     *
     * @return
     */
    [[nodiscard]] sa_family_t family() const noexcept {
        const auto ptr = sockaddr_ptr();
        return ptr ? ptr->sa_family : AF_UNSPEC;
    }

    /**
     *
     * @param n
     * @return
     */
    [[nodiscard]] uint8_t operator[](size_t n) const noexcept {
        const in_addr_t bytes = address();
        return ((const uint8_t*)&bytes)[n];
    }

    /**
     *
     * @return
     */
    [[nodiscard]] std::string to_string() const noexcept {
        char buf[INET_ADDRSTRLEN]{};
        const auto str = inet_ntop(AF_INET, (void*)&(m_addr.sin_addr), buf, INET_ADDRSTRLEN);
        return std::string(str ? str : "<unknown>") + ":" + std::to_string(unsigned(port()));
    }

private:
    storage_t m_addr = {};
};


template<address_family FamilyLHS, address_family FamilyRHS>
inline bool operator==(const socket_address<FamilyLHS>& lhs, const socket_address<FamilyRHS>& rhs) noexcept {
    return lhs.size() == rhs.size() && std::memcmp(lhs.sockaddr_ptr(), rhs.sockaddr_ptr(), lhs.size()) == 0;
}

template<address_family FamilyLHS, address_family FamilyRHS>
inline bool operator!=(const socket_address<FamilyLHS>& lhs, const socket_address<FamilyRHS>& rhs) noexcept {
    return !operator==(lhs, rhs);
}


template<address_family Family>
std::ostream& operator<<(std::ostream& os, const socket_address<Family>& addr) noexcept {
    return (os << addr.to_string());
}

/**
 * Address type aliases
 */
using address_any = socket_address<address_family::any>;
using address_v4  = socket_address<address_family::ipv4>;
using address_v6  = socket_address<address_family::ipv6>;

} // net

/**
 * Hash function for the socket address
 */
template<net::address_family Family>
struct std::hash<net::socket_address<Family>> {
    size_t operator()(const net::socket_address<Family>& addr) const {
        return std::hash<std::string>{}(addr.to_string());
    }
};

static_assert(types::is_std_hashable_v<net::address_any>, "Generic address type is not hashable.");
static_assert(types::is_std_hashable_v<net::address_v4>,  "IPv4 address type is not hashable.");
static_assert(types::is_std_hashable_v<net::address_v6>,  "IPv6 address type is not hashable.");

#endif // SOCKET_ADDRESS_HPP