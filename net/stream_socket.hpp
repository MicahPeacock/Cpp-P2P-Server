#ifndef STREAM_SOCKET_HPP
#define STREAM_SOCKET_HPP

#include "buffer.hpp"
#include "socket.hpp"

namespace net {

/**
 *
 */
template<typename AddrType>
class stream_socket : public socket<AddrType> {
    using base_t = socket<AddrType>;

public:
    static constexpr int COMM_TYPE = SOCK_STREAM;
    static constexpr sa_family_t ADDRESS_FAMILY = AddrType::ADDRESS_FAMILY;
    using address_t = AddrType;

    // Non-copyable
    stream_socket(const stream_socket&) = delete;
    stream_socket& operator=(const stream_socket&) = delete;

    /**
     *
     */
    stream_socket() = default;

    /**
     * Creates
     * @param handle A socket handle from the operating system.
     */
    explicit stream_socket(socket_t handle)
            : base_t(handle) {}

    /**
     *
     * @param sock
     */
    stream_socket(stream_socket&& sock) noexcept
            : base_t(std::move(sock)) {}

    /**
     * Move assignment.
     * @param rhs The other socket to move into this one.
     * @return A reference to this object.
     */
    stream_socket& operator=(stream_socket&& rhs) noexcept {
        base_t::operator=(std::move(rhs));
        return *this;
    }

    /**
     * Creates a stream socket.
	 * @param protocol The particular protocol to be used with the socket.
     * @return a stream socket with the suggested communication characteristics.
     */
    static stream_socket create(int protocol = 0) {
        stream_socket sock(::socket(ADDRESS_FAMILY, COMM_TYPE, protocol));
        if(!sock) sock.clear(base_t::get_last_error());
        return sock;
    }

    /**
     * Creates a stream socket.
	 * @param protocol The particular protocol to be used with the socket.
	 * @return A stream socket
     */
    static std::pair<stream_socket, stream_socket> pair(int protocol = 0) {
        auto pr = base_t::pair(ADDRESS_FAMILY, COMM_TYPE, protocol);
        return std::make_pair<stream_socket, stream_socket>(
                stream_socket_tmpl(pr.first.release()),
                stream_socket_tmpl(pr.second.release()));
    }

    /**
     * Creates a new stream_socket that refers to this one.
	 * This creates a new object with an independent lifetime, but refers
	 * back to this same socket. On most systems, this duplicates the file
	 * handle using the dup() call.
	 * A typical use of this is to have separate threads for reading and
	 * writing the socket. One thread would get the original socket and the
	 * other would get the cloned one.
	 * @return A new stream socket object that refers to the same socket as
	 *  	   this one.
     */
    stream_socket clone() const {
        auto handle = base_t::clone().release();
        return stream_socket(handle);
    }

    /**
     * Reads from the port.
     * @param buf Buffer to acquire the incoming data.
     * @param n The number of bytes to try to read.
     * @return the number of bytes successfully read, or @em -1 on error.
     */
    ssize_t read(const mutable_buffer& payload) const noexcept {
        return base_t::check_return(::recv(base_t::handle(), payload.data(), payload.size(), 0));
    }

    /**
     * Best effort attempts to read the specified number of bytes.
	 * This will make repeated read attempts until all the bytes are read in
	 * or until an error occurs.
     * @param buf Buffer to acquire the incoming data.
     * @param n The number of bytes to try to read.
     * @return the number of bytes successfully read, or @em -1 on error.
     *         If successful, the number of bytes read should be @em n.
     */
    ssize_t read_some(const mutable_buffer& payload) const noexcept {
        size_t  bytes_read = 0;
        ssize_t bytes_remaining = 0;
        auto* bytes = reinterpret_cast<uint8_t*>(payload.data());
        while(bytes_read < payload.size()) {
            if((bytes_remaining = read(bytes + bytes_read, payload.size() - bytes_read)) < 0
                    && base_t::last_error() == EINTR)
                continue;
            if(bytes_remaining <= 0)
                break;
            bytes_read += bytes_remaining;
        }
        return (bytes_read == 0 && bytes_remaining < 0) ? bytes_remaining : ssize_t(bytes_read);
    }

    /**
     * Set a timeout for read operations.
	 * Sets the timeout that the device uses for read operations. Not all
	 * devices support timeouts, so the caller should prepare for failure.
	 * @param to The amount of time to wait for the operation to complete.
	 * @return @em true on success, @em false on failure.
     */
    bool read_timeout(const std::chrono::microseconds& to) const noexcept {
        const auto tv = to_timeval(to);
        return base_t::set_option(SOL_SOCKET, SO_RCVTIMEO, tv);
    }

    /**
     * Set a timeout for read operations.
	 * Sets the timeout that the device uses for read operations. Not all
	 * devices support timeouts, so the caller should prepare for failure.
	 * @param to The amount of time to wait for the operation to complete.
	 * @return @em true on success, @em false on failure.
     */
    template<typename Rep, typename Period>
    bool read_timeout(const std::chrono::duration<Rep, Period>& to) {
        return read_timeout(std::chrono::duration_cast<std::chrono::microseconds>(to));
    }

    /**
     * Writes the buffer to the socket.
     * @param buf The buffer to write.
     * @param n The number of bytes in the buffer.
     * @return The number of bytes successfully written, or @em -1 on error.
     */
    ssize_t write(const const_buffer& payload) const noexcept {
        return base_t::check_return(::send(this->handle(), payload.data(), payload.size(), 0));
    }

    /**
     * Best effort attempt to write the whole buffer to the socket.
     * @param buf The buffer to write.
     * @param n The number of bytes in the buffer.
     * @return The number of bytes successfully written, or @em -1 on error.
     *         If successful, the number of bytes written should be @em n.
     */
    ssize_t write_some(const const_buffer& payload) const noexcept {
        size_t  bytes_written   = 0;
        ssize_t bytes_remaining = 0;
        const auto* bytes = static_cast<const uint8_t*>(payload.data());
        while(bytes_written < payload.size()) {
            if((bytes_remaining = write(bytes + bytes_written, payload.size() - bytes_written)) < 0
                    && base_t::last_error() == EINTR)
                continue;
            if(bytes_written <= 0)
                break;
            bytes_written += bytes_remaining;
        }
        return (bytes_written == 0 && bytes_remaining < 0) ? bytes_remaining : ssize_t(bytes_written);
    }

    /**
     * Set a timeout for write operations.
	 * Sets the timout that the device uses for write operations. Not all
	 * devices support timouts, so the caller should prepare for failure.
	 * @param to The amount of time to wait for the operation to complete.
	 * @return @em true on success, @em false on failure.
     */
    bool write_timeout(const std::chrono::microseconds& to) const noexcept {
        const auto tv = to_timeval(to);
        return base_t::set_option(SOL_SOCKET, SO_SNDTIMEO, tv);
    }

    /**
     * Set a timeout for write operations.
	 * Sets the timout that the device uses for write operations. Not all
	 * devices support timouts, so the caller should prepare for failure.
	 * @param to The amount of time to wait for the operation to complete.
	 * @return @em true on success, @em false on failure.
     */
    template<typename Rep, typename Period>
    bool write_timeout(const std::chrono::duration<Rep, Period>& to) const noexcept {
        return write_timeout(std::chrono::duration_cast<std::chrono::microseconds>(to));
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

#endif //STREAM_SOCKET_HPP