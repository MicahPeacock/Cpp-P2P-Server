#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <cstdlib>

#include <array>
#include <string>
#include <vector>

/**
 * The socket library uses "buffers" to send/receive data.
 * A buffer is simply a non-owning wrapper around a block of bytes which contain both a pointer to the
 * data and the size in bytes.
 */
namespace net {

/**
 * Represents a pointer to mutable data.
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
 * Represents a pointer to immutable data.
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
 * Moves the pointer in the buffer by a given offset.
 * @param buf the mutable_buffer instance.
 * @param n the offset to increment the pointer.
 * @return a new mutable_buffer instance with the moved pointer.
 */
constexpr inline mutable_buffer operator+(const mutable_buffer& b, size_t n) noexcept {
    const size_t offset = n < b.size() ? n : b.size();
    char* new_data = static_cast<char*>(b.data()) + offset;
    size_t new_size = b.size() - offset;
    return { new_data, new_size };
}

/**
 * Moves the pointer in the buffer by a given offset.
 * @param n the offset to increment the pointer.
 * @param buf the mutable_buffer instance.
 * @return a new mutable_buffer instance with the moved pointer.
 */
constexpr inline mutable_buffer operator+(size_t n, const mutable_buffer& b) noexcept {
    return b + n;
}

/**
 * Constructs a mutable buffer an existing mutable buffer instance.
 * @param buf a mutable buffer instance.
 * @return a mutable_buffer instance.
 */
constexpr inline mutable_buffer buffer(const mutable_buffer& b) noexcept {
    return { b };
}

/**
 * Constructs a mutable buffer an existing mutable buffer instance.
 * @param buf a mutable buffer instance.
 * @param max_bytes the maximum number of bytes to save in the buffer.
 * @return a mutable_buffer instance.
 */
constexpr inline mutable_buffer buffer(const mutable_buffer& b, size_t max_bytes) noexcept {
    return { b.data(), b.size() < max_bytes ? b.size() : max_bytes };
}

/**
 * Constructs a mutable buffer from a pointer and a size.
 * @param data a pointer to the data.
 * @param size the number of bytes to save in the buffer.
 * @return a mutable_buffer instance.
 */
constexpr inline mutable_buffer buffer(void* data, size_t size) noexcept {
    return { data, size };
}

/**
 * Constructs a mutable buffer from a C-style array.
 * @param data a C-style array.
 * @return a mutable_buffer instance.
 */
template<typename DataType, size_t N>
constexpr inline mutable_buffer buffer(DataType (&data)[N]) noexcept {
    return { data, N * sizeof(DataType) };
}

/**
 * Constructs a mutable buffer from a C-style array.
 * @param data a C-style array.
 * @param max_bytes the maximum number of bytes to save in the buffer.
 * @return a mutable_buffer instance.
 */
template<typename DataType, size_t N>
constexpr inline mutable_buffer buffer(DataType (&data)[N], size_t max_bytes) noexcept {
    return { data, (N * sizeof(DataType)) < max_bytes ? (N * sizeof(DataType)) : max_bytes };
}

/**
 * Constructs a mutable buffer from a C++ std::array.
 * @param a C++ std::array.
 * @return a mutable_buffer instance.
 */
template<typename DataType, size_t N>
constexpr inline mutable_buffer buffer(std::array<DataType, N>& a) noexcept {
    return { a.data(), a.size() * sizeof(DataType) };
}

/**
 * Constructs a mutable buffer from a C++ std::array.
 * @param a C++ std::array.
 * @param max_bytes the maximum number of bytes to save in the buffer.
 * @return a mutable_buffer instance.
 */
template<typename DataType, size_t N>
constexpr inline mutable_buffer buffer(std::array<DataType, N>& a, size_t max_bytes) noexcept {
    return { a.data(), (a.size() * sizeof(DataType)) < max_bytes ? (a.size() * sizeof(DataType)) : max_bytes };
}

/**
 * Constructs a mutable buffer from a vector.
 * @param v a C++ vector instance.
 * @return a mutable_buffer instance.
 */
template<typename DataType>
inline mutable_buffer buffer(std::vector<DataType>& v) noexcept {
    return { v.data(), v.size() * sizeof(DataType) };
}

/**
 * Constructs a mutable buffer from a vector.
 * @param v a C++ vector instance.
 * @param max_bytes the maximum number of bytes to save in the buffer.
 * @return a mutable_buffer instance.
 */
template<typename DataType>
inline mutable_buffer buffer(std::vector<DataType>& v, size_t max_bytes) noexcept {
    return { v.data(), (v.size() * sizeof(DataType)) < max_bytes ? (v.size() * sizeof(DataType)) : max_bytes };
}

/**
 * Constructs a mutable buffer from a string.
 * @param s a string.
 * @return a mutable_buffer instance.
 */
inline mutable_buffer buffer(std::string& s) noexcept {
    return { s.data(), s.size() };
}

/**
 * Constructs a mutable buffer from a string.
 * @param s a string.
 * @param max_bytes the maximum number of bytes to save in the buffer.
 * @return a mutable_buffer instance.
 */
inline mutable_buffer buffer(std::string& s, size_t max_bytes) noexcept {
    return { s.data(), s.size() < max_bytes ? s.size() : max_bytes };
}


/**
 * Moves the pointer in the buffer by a given offset.
 * @param b
 * @param n
 * @return a new const_buffer instance with the moved pointer.
 */
constexpr inline const_buffer operator+(const const_buffer& b, size_t n) noexcept {
    size_t offset = n < b.size() ? n : b.size();
    const char* new_data = static_cast<const char*>(b.data()) + offset;
    size_t new_size = b.size() - offset;
    return { new_data, new_size };
}

/**
 * Moves the pointer in the buffer by a given offset.
 * @param n
 * @param b
 * @return a new const_buffer instance with the moved pointer.
 */
constexpr inline const_buffer operator+(size_t n, const const_buffer& b) noexcept {
    return b + n;
}

/**
 * Constructs a const buffer an existing const buffer instance.
 * @param b
 * @return a const_buffer instance.
 */
constexpr inline const_buffer buffer(const const_buffer& b) noexcept {
    return { b };
}

/**
 * Constructs a const buffer an existing const buffer instance.
 * @param b
 * @param max_bytes the maximum number of bytes to save in the buffer.
 * @return a const_buffer instance.
 */
constexpr inline const_buffer buffer(const const_buffer& b, size_t max_bytes) noexcept {
    return { b.data(), b.size() < max_bytes ? b.size() : max_bytes };
}

/**
 * Constructs a const buffer from a pointer and a size.
 * @param data
 * @param size the number of bytes to save in the buffer.
 * @return a const_buffer instance.
 */
constexpr inline const_buffer buffer(const void* data, size_t size) noexcept {
    return { data, size };
}

/**
 * Constructs a const buffer from a C-style array.
 * @param data a C-style array.
 * @return a const_buffer instance.
 */
template<typename DataType, size_t N>
constexpr inline const_buffer buffer(const DataType (&data)[N]) noexcept {
    return { data, N };
}

/**
 * Constructs a const buffer from a C-style array.
 * @param data a C-style array.
 * @param max_bytes the maximum number of bytes to save in the buffer.
 * @return a const_buffer instance.
 */
template<typename DataType, size_t N>
constexpr inline const_buffer buffer(const DataType (&data)[N], size_t max_bytes) noexcept {
    return { data, N < max_bytes ? N : max_bytes };
}

/**
 * Constructs a const buffer from a C++ std::array.
 * @param a a C++ std::array instance.
 * @return a const_buffer instance.
 */
template<typename DataType, size_t N>
constexpr inline const_buffer buffer(const std::array<DataType, N>& a) noexcept {
    return { a.data(), a.size() * sizeof(DataType) };
}

/**
 * Constructs a const buffer from a C++ std::array.
 * @param a a C++ std::array instance.
 * @param max_bytes the maximum number of bytes to save in the buffer.
 * @return a const_buffer instance.
 */
template<typename DataType, size_t N>
constexpr inline const_buffer buffer(const std::array<DataType, N>& a, size_t max_bytes) noexcept {
    return { a.data(), (a.size() * sizeof(DataType)) < max_bytes ? (a.size() * sizeof(DataType)) : max_bytes };
}

/**
 * Constructs a const buffer from a C++ std::array.
 * @param a a C++ std::array instance.
 * @return a const_buffer instance.
 */
template<typename DataType, size_t N>
constexpr inline const_buffer buffer(std::array<const DataType, N>& a) noexcept {
    return { a.data(), a.size() * sizeof(DataType) };
}

/**
 * Constructs a const buffer from a C++ std::array.
 * @param a a C++ std::array instance.
 * @param max_bytes the maximum number of bytes to save in the buffer.
 * @return a const_buffer instance.
 */
template<typename DataType, size_t N>
constexpr inline const_buffer buffer(std::array<const DataType, N>& a, size_t max_bytes) noexcept {
    return { a.data(), (a.size() * sizeof(DataType)) < max_bytes ? (a.size() * sizeof(DataType)) : max_bytes };
}

/**
 * Constructs a const buffer from a vector.
 * @param v a vector instance.
 * @return a const_buffer instance.
 */
template<typename DataType>
inline const_buffer buffer(const std::vector<DataType>& v) noexcept {
    return { v.data(), v.size() * sizeof(DataType) };
}

/**
 * Constructs a const buffer from a vector.
 * @param v a vector instance.
 * @param max_bytes the maximum number of bytes to save in the buffer.
 * @return a const_buffer instance.
 */
template<typename DataType>
inline const_buffer buffer(const std::vector<DataType>& v, size_t max_bytes) noexcept {
    return { v.data(), (v.size() * sizeof(DataType)) < max_bytes ? (v.size() * sizeof(DataType)) : max_bytes };
}

/**
 * Constructs a const buffer from a vector.
 * @param v a vector instance.
 * @return a const_buffer instance.
 */
template<typename DataType>
inline const_buffer buffer(std::vector<const DataType>& v) noexcept {
    return { v.data(), v.size() * sizeof(DataType) };
}

/**
 * Constructs a const buffer from a vector.
 * @param v a vector instance.
 * @param max_bytes the maximum number of bytes to save in the buffer.
 * @return a const_buffer instance.
 */
template<typename DataType>
inline const_buffer buffer(std::vector<const DataType>& v, size_t max_bytes) noexcept {
    return { v.data(), (v.size() * sizeof(DataType)) < max_bytes ? (v.size() * sizeof(DataType)) : max_bytes };
}

/**
 * Constructs a const buffer from a string.
 * @param s a string.
 * @return a const_buffer instance.
 */
inline const_buffer buffer(const std::string& s) noexcept {
    return { s.data(), s.size() };
}

/**
 * Constructs a const buffer from a string.
 * @param s a string.
 * @param max_bytes the maximum number of bytes to save in the buffer.
 * @return a const_buffer instance.
 */
inline const_buffer buffer(const std::string& s, size_t max_bytes) noexcept {
    return { s.data(), s.size() < max_bytes ? s.size() : max_bytes };
}

} // net

#endif // BUFFER_HPP
