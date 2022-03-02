#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <type_traits>
#include <vector>


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


namespace clocks {

using namespace std::chrono;

using time_type = time_point<system_clock, seconds>;

time_type get_current_time() noexcept {
    return time_point_cast<seconds>(system_clock::now());
}

std::string get_current_time_str() noexcept {
    const auto now  = system_clock::now();
    const auto time = system_clock::to_time_t(now);
    const auto tm   = std::localtime(&time);
    char strtime[32] = {};
    std::strftime(strtime, sizeof(strtime), "%Y-%m-%d %H:%M:%S", tm);
    return strtime;
}

} // clocks


namespace strings {

template<typename... Args>
static std::string join(const std::string& delimiter, Args&&... args) {
    std::vector<std::string> strings = { args... };
    std::accumulate(std::next(strings.begin()), strings.end(), strings[0], [&](const auto& a, const auto& b) {
        return a + delimiter + b;
    });
    return strings[0];
}

std::pair<std::string, std::string> split(const std::string& s, const char delimiter) {
    return std::make_pair(s.substr(0, s.find(delimiter)), s.substr(s.find(delimiter) + 1, s.size()));
}

bool contains(const std::string& s, std::string&& target) {
    return s.find(target) != std::string::npos;
}

bool ends_with(const std::string& s, const std::string& ending) {
    if(ending.size() > s.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), s.rbegin());
}

} // strings

#endif //UTILS_HPP