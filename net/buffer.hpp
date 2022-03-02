#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <cstdlib>
#include <array>
#include <string>
#include <vector>

namespace net {

/**
 *
 */
class mutable_buffer {
public:
    constexpr mutable_buffer()
            : m_data(nullptr), m_size(0) {}

    constexpr mutable_buffer(void* data, size_t size)
            : m_data(data), m_size(size) {}

    [[nodiscard]] constexpr void* data() const noexcept { return m_data; }
    [[nodiscard]] constexpr size_t size() const noexcept { return m_size; }

    constexpr mutable_buffer& operator+=(size_t n) noexcept {
        size_t offset = n < m_size ? n : m_size;
        m_data = static_cast<char*>(m_data) + offset;
        m_size -= offset;
        return *this;
    }

private:
    void* m_data;
    size_t m_size;
};


/**
 *
 */
class const_buffer {
public:
    constexpr const_buffer() noexcept
            : m_data(nullptr), m_size(0) {}

    constexpr const_buffer(const void* data, size_t size) noexcept
            : m_data(data), m_size(size) {}

    constexpr explicit const_buffer(const mutable_buffer& buf) noexcept
            : m_data(buf.data()), m_size(buf.size()) {}

    [[nodiscard]] constexpr const void* data() const noexcept { return m_data; }
    [[nodiscard]] constexpr size_t size() const noexcept { return m_size; }

    constexpr const_buffer& operator+=(size_t n) noexcept {
        size_t offset = n < m_size ? n : m_size;
        m_data = static_cast<const char*>(m_data) + offset;
        m_size -= offset;
        return *this;
    }

private:
    const void* m_data;
    size_t m_size;
};


/**
 *
 * @param buf
 * @param n
 * @return
 */
constexpr inline mutable_buffer operator+(const mutable_buffer& b, size_t n) noexcept {
    const size_t offset = n < b.size() ? n : b.size();
    char* new_data = static_cast<char*>(b.data()) + offset;
    size_t new_size = b.size() - offset;
    return { new_data, new_size };
}

/**
 *
 * @param n
 * @param buf
 * @return
 */
constexpr inline mutable_buffer operator+(size_t n, const mutable_buffer& b) noexcept {
    return b + n;
}

/**
 *
 * @param buf
 * @return
 */
constexpr inline mutable_buffer buffer(const mutable_buffer& b) noexcept {
    return { b };
}

/**
 *
 * @param buf
 * @param max_bytes
 * @return
 */
constexpr inline mutable_buffer buffer(const mutable_buffer& b, size_t max_bytes) noexcept {
    return { b.data(), b.size() < max_bytes ? b.size() : max_bytes };
}

/**
 *
 * @param data
 * @param size
 * @return
 */
constexpr inline mutable_buffer buffer(void* data, size_t size) noexcept {
    return { data, size };
}

/**
 *
 * @param data
 * @return
 */
template<typename DataType, size_t N>
constexpr inline mutable_buffer buffer(DataType (&data)[N]) noexcept {
    return { data, N * sizeof(DataType) };
}

/**
 *
 * @param data
 * @param max_bytes
 * @return
 */
template<typename DataType, size_t N>
constexpr inline mutable_buffer buffer(DataType (&data)[N], size_t max_bytes) noexcept {
    return { data, (N * sizeof(DataType)) < max_bytes ? (N * sizeof(DataType)) : max_bytes };
}

/**
 *
 * @param a
 * @return
 */
template<typename DataType, size_t N>
constexpr inline mutable_buffer buffer(std::array<DataType, N>& a) noexcept {
    return { a.data(), a.size() * sizeof(DataType) };
}

/**
 *
 * @param a
 * @param max_bytes
 * @return
 */
template<typename DataType, size_t N>
constexpr inline mutable_buffer buffer(std::array<DataType, N>& a, size_t max_bytes) noexcept {
    return { a.data(), (a.size() * sizeof(DataType)) < max_bytes ? (a.size() * sizeof(DataType)) : max_bytes };
}

/**
 *
 * @param v
 * @return
 */
template<typename DataType>
inline mutable_buffer buffer(std::vector<DataType>& v) noexcept {
    return { v.data(), v.size() * sizeof(DataType) };
}

/**
 *
 * @param v
 * @param max_bytes
 * @return
 */
template<typename DataType>
inline mutable_buffer buffer(std::vector<DataType>& v, size_t max_bytes) noexcept {
    return { v.data(), (v.size() * sizeof(DataType)) < max_bytes ? (v.size() * sizeof(DataType)) : max_bytes };
}

/**
 *
 * @param s
 * @return
 */
inline mutable_buffer buffer(std::string& s) noexcept {
    return { s.data(), s.size() };
}

/**
 *
 * @param s
 * @param max_bytes
 * @return
 */
inline mutable_buffer buffer(std::string& s, size_t max_bytes) noexcept {
    return { s.data(), s.size() < max_bytes ? s.size() : max_bytes };
}


/**
 *
 * @param b
 * @param n
 * @return
 */
constexpr inline const_buffer operator+(const const_buffer& b, size_t n) noexcept {
    size_t offset = n < b.size() ? n : b.size();
    const char* new_data = static_cast<const char*>(b.data()) + offset;
    size_t new_size = b.size() - offset;
    return { new_data, new_size };
}

/**
 *
 * @param n
 * @param b
 * @return
 */
constexpr inline const_buffer operator+(size_t n, const const_buffer& b) noexcept {
    return b + n;
}

/**
 *
 * @param b
 * @return
 */
constexpr inline const_buffer buffer(const const_buffer& b) noexcept {
    return { b };
}

/**
 *
 * @param b
 * @param max_bytes
 * @return
 */
constexpr inline const_buffer buffer(const const_buffer& b, size_t max_bytes) noexcept {
    return { b.data(), b.size() < max_bytes ? b.size() : max_bytes };
}

/**
 *
 * @param data
 * @param size
 * @return
 */
constexpr inline const_buffer buffer(const void* data, size_t size) noexcept {
    return { data, size };
}

/**
 *
 * @param data
 * @return
 */
template<typename DataType, size_t N>
constexpr inline const_buffer buffer(const DataType (&data)[N]) noexcept {
    return { data, N };
}

/**
 *
 * @param data
 * @param max_bytes
 * @return
 */
template<typename DataType, size_t N>
constexpr inline const_buffer buffer(const DataType (&data)[N], size_t max_bytes) noexcept {
    return { data, N < max_bytes ? N : max_bytes };
}

/**
 *
 * @param a
 * @return
 */
template<typename DataType, size_t N>
constexpr inline const_buffer buffer(const std::array<DataType, N>& a) noexcept {
    return { a.data(), a.size() * sizeof(DataType) };
}

/**
 *
 * @param a
 * @param max_bytes
 * @return
 */
template<typename DataType, size_t N>
constexpr inline const_buffer buffer(const std::array<DataType, N>& a, size_t max_bytes) noexcept {
    return { a.data(), (a.size() * sizeof(DataType)) < max_bytes ? (a.size() * sizeof(DataType)) : max_bytes };
}

/**
 *
 * @param a
 * @return
 */
template<typename DataType, size_t N>
constexpr inline const_buffer buffer(std::array<const DataType, N>& a) noexcept {
    return { a.data(), a.size() * sizeof(DataType) };
}

/**
 *
 * @param a
 * @param max_bytes
 * @return
 */
template<typename DataType, size_t N>
constexpr inline const_buffer buffer(std::array<const DataType, N>& a, size_t max_bytes) noexcept {
    return { a.data(), (a.size() * sizeof(DataType)) < max_bytes ? (a.size() * sizeof(DataType)) : max_bytes };
}

/**
 *
 * @param v
 * @return
 */
template<typename DataType>
inline const_buffer buffer(const std::vector<DataType>& v) noexcept {
    return { v.data(), v.size() * sizeof(DataType) };
}

/**
 *
 * @param v
 * @param max_bytes
 * @return
 */
template<typename DataType>
inline const_buffer buffer(const std::vector<DataType>& v, size_t max_bytes) noexcept {
    return { v.data(), (v.size() * sizeof(DataType)) < max_bytes ? (v.size() * sizeof(DataType)) : max_bytes };
}

/**
 *
 * @param v
 * @return
 */
template<typename DataType>
inline const_buffer buffer(std::vector<const DataType>& v) noexcept {
    return { v.data(), v.size() * sizeof(DataType) };
}

/**
 *
 * @param v
 * @param max_bytes
 * @return
 */
template<typename DataType>
inline const_buffer buffer(std::vector<const DataType>& v, size_t max_bytes) noexcept {
    return { v.data(), (v.size() * sizeof(DataType)) < max_bytes ? (v.size() * sizeof(DataType)) : max_bytes };
}

/**
 *
 * @param s
 * @return
 */
inline const_buffer buffer(const std::string& s) noexcept {
    return { s.data(), s.size() };
}

/**
 *
 * @param s
 * @param max_bytes
 * @return
 */
inline const_buffer buffer(const std::string& s, size_t max_bytes) noexcept {
    return { s.data(), s.size() < max_bytes ? s.size() : max_bytes };
}

} // net

#endif // BUFFER_HPP
