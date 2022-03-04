#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <type_traits>
#include <vector>


/**
 * Compile-time helper methods that assert types are properly hashable.
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
 * Helper methods that handle chrono time points and datetime instances.
 */
namespace clocks {

using namespace std::chrono;

using time_type = time_point<system_clock, seconds>;

/**
 * Gets a time_point of the current time.
 * @return a time_point of the current time.
 */
time_type get_current_time() noexcept {
    return time_point_cast<seconds>(system_clock::now());
}

/**
 * Gets a string of the current date and time.
 * @return a string of the current date and time.
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
 * String helper methods
 */
namespace strings {

/**
 * Given a delimiter and a variadic list of strings, will append all given strings separated by the delimiter into a
 * single string.
 * @param delimiter the delimiter to place between the strings.
 * @param args a variadic list of strings.
 * @return a single joined string.
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
 * Given a string and a delimiter, will split the string into a pair of both before and after the delimiter.
 * @param s the string to split.
 * @param delimiter the character to search for.
 * @return a pair where the first element is the substring before the delimiter, and the second element is after the delimiter.
 */
std::pair<std::string, std::string> split(const std::string& s, const char delimiter) {
    return std::make_pair(s.substr(0, s.find(delimiter)), s.substr(s.find(delimiter) + 1, s.size()));
}

/**
 * Given a string and a target string, will check if the given string contains the target substring.
 * @param s the string to search.
 * @param target the target substring.
 * @return true if the target substring exists in the given string, false otherwise.
 */
bool contains(const std::string& s, std::string&& target) {
    return s.find(target) != std::string::npos;
}

/**
 * Given a string and a specified ending, will check if the given string ends with the given character sequence.
 * @param s the string to search.
 * @param ending the character sequence.
 * @return true if the character sequence matches the end of the string, false otherwise.
 */
bool ends_with(const std::string& s, const std::string& ending) {
    if(ending.size() > s.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), s.rbegin());
}

/**
 * Given a string and a specified ending, will check if the given string starts with the given character sequence.
 * @param s the string to search.
 * @param beginning the character sequence.
 * @return true if the character sequence matches the beginning of the string, false otherwise.
 */
bool starts_with(const std::string& s, const std::string& beginning) {
    if(beginning.size() > s.size()) return false;
    return std::equal(beginning.begin(), beginning.end(), s.begin());
}

/**
 * Trims all whitespace from the beginning of a given string (in-place).
 * @param s the string to trim.
 */
void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {
        return !std::isspace(c);
    }));
}

/**
 * Trims all whitespace from the end of a given string (in-place).
 * @param s the string to trim.
 */
void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base(), s.end());
}

/**
 * Removes all leading and trailing whitespace from a given string.
 * @param s the string to trim.
 * @return the trimmed string.
 */
std::string trim(const std::string& s) {
    std::string ret = s;
    ltrim(ret);
    rtrim(ret);
    return ret;
}

} // strings

#endif //UTILS_HPP
