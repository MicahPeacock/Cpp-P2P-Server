#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "socket_address.hpp"
#include <chrono>
#include <string>
#include <fcntl.h>

namespace net {

using socket_t = int;
using error_t  = int;
constexpr socket_t INVALID_SOCKET = -1;

using namespace std::chrono;

/**
 *
 * @param duration
 * @return
 */
constexpr timeval to_timeval(const microseconds& duration) {
    const seconds sec = duration_cast<seconds>(duration);
    timeval tv = {};
    tv.tv_sec  = time_t(sec.count());
    tv.tv_usec = suseconds_t(duration_cast<microseconds>(duration - sec).count());
    return tv;
}

/**
 *
 * @tparam Rep
 * @tparam Period
 * @param duration
 * @return
 */
template<typename Rep, typename Period>
constexpr timeval to_timeval(const duration<Rep, Period>& duration) {
    return to_timeval(duration_cast<microseconds>(duration));
}


/**
 *
 */
template<typename AddrType>
class socket {
public:
    using address_t = AddrType;

    // Non-copyable
    socket(const socket&) = delete;
    socket& operator=(const socket&) = delete;

    /**
     *
     */
    socket() : m_handle(INVALID_SOCKET), m_last_error(0) {}

    /**
     *
     * @param handle
     */
    explicit socket(socket_t handle)
            : m_handle(handle), m_last_error(0) {}

    /**
     *
     * @param sock
     */
    socket(socket&& sock) noexcept
            : m_handle(sock.m_handle), m_last_error(sock.m_last_error) {
        sock.m_handle = INVALID_SOCKET;
        sock.m_last_error = 0;
    }

    /**
     *
     * @param sock
     * @return
     */
    socket& operator=(socket&& sock) noexcept {
        std::swap(m_handle, sock.m_handle);
        m_last_error = sock.m_last_error;
        return *this;
    }

    /**
     *
     */
    virtual ~socket() { close(); }


    /**
     *
     * @param domain
     * @param type
     * @param protocol
     * @return
     */
    static socket create(int domain, int type, int protocol = 0) noexcept {
        socket sock(::socket(domain, type, protocol));
        if(!sock) sock.clear(get_last_error());
        return sock;
    }

    /**
     *
     * @param domain
     * @param type
     * @param protocol
     * @return
     */
    static std::pair<socket, socket> pair(int domain, int type, int protocol = 0) {
        socket sock0, sock1;
        int sv[2];
        int ret = ::socketpair(domain, type, protocol, sv);
        if(ret == 0) {
            sock0.reset(sv[0]);
            sock1.reset(sv[1]);
        } else {
            error_t error = get_last_error();
            sock0.clear(error);
            sock1.clear(error);
        }
        return std::make_pair<socket, socket>(std::move(sock0), std::move(sock1));
    }

    /**
     *
     * @param error_code
     * @return
     */
    static std::string str_error(error_t error_code) {
        return system_error::str_error(error_code);
    }

    /**
     *
     * @return
     */
    [[nodiscard]] constexpr bool is_open() const noexcept {
        return m_handle != INVALID_SOCKET;
    }

    /**
     *
     * @return
     */
    [[nodiscard]] address_t address() const {
        auto addr_storage = typename address_t::storage_t{};
        socklen_t len = sizeof(typename address_t::storage_t);

        if(!check_return_bool(::getsockname(m_handle,
                reinterpret_cast<sockaddr*>(&addr_storage), &len)))
            return address_t{};
        return address_t(addr_storage);
    }

    /**
     *
     * @return
     */
    [[nodiscard]] address_t peer_address() const {
        auto addr_storage = typename address_t::storage_t{};
        socklen_t len = sizeof(typename address_t::storage_t);

        if(!check_return_bool(::getpeername(m_handle,
                reinterpret_cast<sockaddr*>(&addr_storage), &len)))
            return address_t{};
        return address_t(addr_storage);
    }

    /**
     *
     * @return
     */
    [[nodiscard]] socket_t handle() const noexcept { return m_handle; }

    /**
     *
     * @return
     */
    [[nodiscard]] sa_family_t family() const noexcept { return address().family(); }

    /**
     *
     * @return
     */
    [[nodiscard]] error_t last_error() const noexcept { return m_last_error; }

    /**
     *
     * @return
     */
    [[nodiscard]] std::string last_error_str() const { return str_error(m_last_error); }

    bool operator!() const noexcept {
        return m_handle == INVALID_SOCKET || m_last_error != 0;
    }

    explicit operator bool() const noexcept {
        return m_handle != INVALID_SOCKET && m_last_error == 0;
    }

    /**
     *
     * @param addr
     * @return
     */
    bool bind(const address_t& addr) noexcept {
        return check_return_bool(::bind(m_handle, addr.sockaddr_ptr(), addr.size()));
    }

    socket clone() const {
        socket_t h = ::dup(m_handle);
        return socket(h);
    }

    /**
     *
     * @param val
     */
    void clear(error_t val = 0) noexcept {
        m_last_error = val;
    }

    /**
     *
     * @return
     */
    socket_t release() noexcept {
        const socket_t h = m_handle;
        m_handle = INVALID_SOCKET;
        return h;
    }

    /**
     *
     * @param handle
     */
    void reset(socket_t handle = INVALID_SOCKET) noexcept {
        socket_t old_handle = m_handle;
        m_handle = handle;
        if(old_handle != INVALID_SOCKET)
            close(old_handle);
        clear();
    }

    /**
     *
     * @param level
     * @param option_name
     * @param option_val
     * @param option_len
     * @return
     */
    bool get_option(int level, int option_name, void* option_val, socklen_t* option_len) const noexcept {
        return check_return_bool(::getsockopt(m_handle, level, option_name, option_val, option_len));
    }

    /**
     *
     * @tparam Type
     * @param level
     * @param option_name
     * @param val
     * @return
     */
    template<class Type>
    bool get_option(int level, int option_name, Type* val) const noexcept {
        socklen_t len = sizeof(Type);
        return get_option(level, option_name, (void*)val, &len);
    }

    /**
     *
     * @param level
     * @param option_name
     * @param option_val
     * @param option_len
     * @return
     */
    bool set_option(int level, int option_name, const void* option_val, socklen_t option_len) const noexcept {
        return check_return_bool(::setsockopt(m_handle, level, option_name, option_val, option_len));
    }

    /**
     *
     * @tparam Type
     * @param level
     * @param option_name
     * @param val
     * @return
     */
    template<class Type>
    bool set_option(int level, int option_name, const Type& val) const noexcept {
        return set_option(level, option_name, (void*)&val, sizeof(Type));
    }

    /**
     *
     * @param on
     * @return
     */
    bool set_non_blocking(bool on = true) const noexcept {
        int flags = ::fcntl(m_handle, F_GETFL, 0);
        if(flags == -1) {
            set_last_error();
            return false;
        }
        flags = on ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
        if(::fcntl(m_handle, F_SETFL, flags) == -1) {
            set_last_error();
            return false;
        }
        return true;
    }

    /**
     *
     * @return
     */
    bool close() noexcept {
        if(m_handle != INVALID_SOCKET) {
            if(!close(release())) {
                set_last_error();
                return false;
            }
        }
        return true;
    }

protected:
    /**
     *
     * @return
     */
    static error_t get_last_error() noexcept {
        error_t error = errno;
        return error;
    }

    /**
     *
     */
    void set_last_error() noexcept {
        m_last_error = get_last_error();
    }

    /**
     *
     * @return
     */
    bool close_on_error() noexcept {
        close(release());
        return false;
    }

    /**
     *
     * @tparam Type
     * @param return_val
     * @return
     */
    template<typename Type>
    Type check_return(Type return_val) const noexcept {
        m_last_error = (return_val < 0) ? get_last_error() : 0;
        return return_val;
    }

    /**
     *
     * @tparam Type
     * @param return_val
     * @return
     */
    template<typename Type>
    bool check_return_bool(Type return_val) const noexcept {
        m_last_error = (return_val < 0) ? get_last_error() : 0;
        return return_val >= 0;
    }

    /**
     *
     * @param return_val
     * @return
     */
    socket_t check_socket(socket_t return_val) const noexcept {
        m_last_error = (return_val == INVALID_SOCKET) ? get_last_error() : 0;
        return return_val;
    }

    /**
     *
     * @param return_val
     * @return
     */
    bool check_socket_bool(socket_t return_val) const noexcept {
        m_last_error = (return_val == INVALID_SOCKET) ? get_last_error() : 0;
        return return_val != INVALID_SOCKET;
    }

private:
    /**
     *
     * @param handle
     * @return
     */
    static bool close(socket_t handle) noexcept {
        return ::close(handle) >= 0;
    }

    socket_t m_handle = INVALID_SOCKET;
    mutable error_t m_last_error = {};
};

} // net

#endif // SOCKET_HPP
