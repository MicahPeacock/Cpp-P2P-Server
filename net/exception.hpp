#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#include <cerrno>
#include <cstring>
#include <netdb.h>

#include <exception>
#include <stdexcept>
#include <string>

namespace net {

class system_error : public std::runtime_error {
public:
    system_error()
            : system_error(errno) {}
    explicit system_error(int error_code)
            : std::runtime_error(str_error(error_code)), m_error_code(error_code) {}

    static std::string str_error(int error_code) {
        char buf[1024] = { '\x00' };
        const auto str = strerror_r(error_code, buf, sizeof(buf));
        return str ? std::string(str) : "";
    }
    [[nodiscard]] constexpr int error() const noexcept { return m_error_code; }

private:
    int m_error_code;
};


class address_error : public std::runtime_error {
public:
    address_error(int error_code, std::string hostname)
        : std::runtime_error(gai_strerror(error_code)), m_hostname(std::move(hostname)), m_error_code(error_code) {}

    [[nodiscard]] const std::string& hostname() const { return m_hostname; }
    [[nodiscard]] constexpr int error() const { return m_error_code; }

private:
    std::string m_hostname;
    int m_error_code;
};


class socket_exception : public std::exception {
public:
    explicit socket_exception(std::string reason) : m_reason(std::move(reason)) {}

    [[nodiscard]] const char* what() const noexcept override {
        return m_reason.c_str();
    }

private:
    const std::string m_reason;
};

} // net

#endif // EXCEPTION_HPP
