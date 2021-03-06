/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "string.hpp"

namespace util::string {
    std::string_view trim(std::string_view src, std::string_view whitespace) {
        std::size_t begin = src.find_first_not_of(whitespace);
        if (begin == std::string::npos) {
            return { };
        }
        std::size_t end = src.find_last_not_of(whitespace);
        return src.substr(begin, end - begin + 1);
    }

    std::vector<std::string> split(std::string_view src, char delim) {
        std::vector<std::string> tokens;
        std::size_t prev = 0;
        std::size_t pos = 0;
        while ((pos = src.find(delim, prev)) != std::string::npos) {
            tokens.emplace_back(src.substr(0, pos - prev));
            prev = pos + 1;
        }
        // Push everything else to the end of the string
        tokens.emplace_back(src.substr(prev));
        return tokens;
    }

    std::vector<std::string> split(std::string_view src, std::string_view delim) {
        std::vector<std::string> tokens;
        std::size_t delim_size = delim.length();
        std::size_t prev = 0;
        std::size_t pos = 0;
        while ((pos = src.find(delim, prev)) != std::string::npos) {
            tokens.emplace_back(src.substr(0, pos - prev));
            prev = pos + delim_size;
        }
        // Push everything else to the end of the string
        tokens.emplace_back(src.substr(prev));
        return tokens;
    }

    std::vector<std::string> split_trim(std::string_view src, char delim, std::string_view whitespace) {
        std::vector<std::string> tokens;
        std::size_t prev = 0;
        std::size_t pos = 0;
        while ((pos = src.find(delim, prev)) != std::string::npos) {
            std::size_t begin = src.find_first_not_of(whitespace, prev);
            // The whole entry is whitespace
            // Unless delim contains whitespace, begin is guaranteed to be 0 <= begin <= pos
            if (begin >= pos) {
                prev = begin == std::string::npos ? begin : begin + 1;
            }
            else {
                // This would fail if pos == 0, because the whole string would be searched
                // However, the previous if statement assures that begin < pos
                // so if pos == 0, begin == 0 because it is the first non-whitespace character, so this statement is never reached
                std::size_t end = src.find_last_not_of(whitespace, pos - 1);
                // prev <= begin <= end < pos
                tokens.emplace_back(src.substr(begin, end - begin + 1));
                prev = pos + 1;
            }
        }
        // Push everything else to the end of the string, if it's not whitespace
        std::size_t begin = src.find_first_not_of(whitespace, prev);
        if (begin != std::string::npos) {
            std::size_t end = src.find_last_not_of(whitespace);
            tokens.emplace_back(src.substr(begin, end - begin + 1));
        }
        return tokens;
    }

    std::string lowercase(std::string_view src) {
        std::string out;
        std::transform(src.begin(), src.end(), std::back_inserter(out),
            [](char c) { return std::tolower(c); });
        return out;
    }

    bool ends_with(std::string_view str, std::string_view suffix) {
        return str.size() >= suffix.size() && std::equal(str.begin() + str.size() - suffix.size(), str.end(), suffix.begin(), suffix.end());
    }

    bool iequals_fn(std::string_view a, std::string_view b) {
        return std::equal(a.begin(), a.end(), b.begin(), b.end(),
            [](char a, char b) {
                return std::tolower(a) == std::tolower(b);
            });
    }

    bool iless::operator()(std::string_view a, std::string_view b) const {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
            [](char a, char b) {
                return std::tolower(a) < std::tolower(b);
            });
    }

    bool iequals::operator()(std::string_view a, std::string_view b) const {
        return iequals_fn(a, b);
    }

    std::size_t ihash::operator()(std::string_view s) const {
        return hasher(lowercase(s));
    }

    std::size_t parse_hexadecimal(std::string_view src) {
        std::size_t out;
        std::stringstream ss;
        ss << std::hex << src;
        if (!(ss >> out) || !(ss >> std::ws).eof()) {
            throw std::bad_cast { };
        }
        return out;
    }
}