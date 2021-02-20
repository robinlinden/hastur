#ifndef UTIL_BASE_PARSER_H_
#define UTIL_BASE_PARSER_H_

#include <cctype>
#include <concepts>
#include <cstddef>
#include <string_view>

namespace util {

template<typename T>
concept Predicate = std::predicate<T, char>;

class BaseParser {
public:
    BaseParser(std::string_view input) : input_{input} {}

    constexpr char peek() const {
        return input_[pos_];
    }

    constexpr std::string_view peek(std::size_t chars) const {
        return input_.substr(pos_, chars);
    }

    constexpr bool starts_with(std::string_view prefix) const {
        return peek(prefix.size()) == prefix;
    }

    constexpr bool is_eof() const {
        return pos_ >= input_.size();
    }

    constexpr char consume_char() {
        return input_[pos_++];
    }

    constexpr void advance(std::size_t n) {
        pos_ += n;
    }

    template<Predicate T>
    constexpr std::string_view consume_while(T const &pred) {
        std::size_t start = pos_;
        while (pred(input_[pos_])) { ++pos_; }
        return input_.substr(start, pos_ - start);
    }

    constexpr void skip_whitespace() {
        while (!is_eof() && std::isspace(static_cast<unsigned char>(peek()))) { advance(1); }
    }

private:
    std::string_view input_;
    std::size_t pos_{0};
};

} // namespace util

#endif
