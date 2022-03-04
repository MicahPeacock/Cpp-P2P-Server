#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <type_traits>
#include <vector>


/**
 *
 */
namespace types {

template<typename T, typename = std::void_t<>>
struct is_std_hashable : std::false_type {
};

template<typename T>
struct is_std_hashable<T, std::void_t<decltype(std::declval<std::hash<T>>()(std::declval<T>()))>> : std::true_type {
};

template<typename T>
constexpr bool is_std_hashable_v = is_std_hashable<T>::value;

} // types


/**
 *
 */
namespace clocks {

using namespace std::chrono;

using time_type = time_point<system_clock, seconds>;

/**
 *
 * @return
 */
time_type get_current_time() noexcept {
    return time_point_cast<seconds>(system_clock::now());
}

/**
 *
 * @return
 */
std::string get_current_time_str() noexcept {
    const auto now  = system_clock::now();
    const auto time = system_clock::to_time_t(now);
    const auto tm   = std::localtime(&time);
    char strtime[32] = {};
    std::strftime(strtime, sizeof(strtime), "%Y-%m-%d %H:%M:%S", tm);
    return strtime;
}

} // clocks


/**
 *
 */
namespace strings {

/**
 *
 * @param delimiter
 * @param args
 * @return
 */
template<typename... Args>
std::string join(const std::string& delimiter, Args&&... args) {
    std::vector<std::string> strings = { args... };
    std::accumulate(std::next(strings.begin()), strings.end(), strings[0], [&](const auto& a, const auto& b) {
        return a + delimiter + b;
    });
    return strings[0];
}

/**
 *
 * @param s
 * @param delimiter
 * @return
 */
std::pair<std::string, std::string> split(const std::string& s, const char delimiter) {
    return std::make_pair(s.substr(0, s.find(delimiter)), s.substr(s.find(delimiter) + 1, s.size()));
}

/**
 *
 * @param s
 * @param target
 * @return
 */
bool contains(const std::string& s, std::string&& target) {
    return s.find(target) != std::string::npos;
}

/**
 *
 * @param s
 * @param ending
 * @return
 */
bool ends_with(const std::string& s, const std::string& ending) {
    if(ending.size() > s.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), s.rbegin());
}

bool starts_with(const std::string& s, const std::string& beginning) {
    if(beginning.size() > s.size()) return false;
    return std::equal(beginning.begin(), beginning.end(), s.begin());
}

void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {
        return !std::isspace(c);
    }));
}

void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base(), s.end());
}

std::string trim(const std::string& s) {
    std::string ret = s;
    ltrim(ret);
    rtrim(ret);
    return ret;
}

} // strings

#endif //UTILS_HPP
