// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JSON_JSON_H_
#define JSON_JSON_H_

#include "unicode/util.h"
#include "util/from_chars.h"
#include "util/string.h"

#include <tl/expected.hpp>

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace json {

enum class Error : std::uint8_t {
    InvalidEscape,
    InvalidKeyword,
    InvalidNumber,
    NestingLimitReached,
    TrailingGarbage,
    UnexpectedCharacter,
    UnexpectedControlCharacter,
    UnexpectedEof,
    UnpairedSurrogate,
};

constexpr std::string_view to_string(Error e) {
    switch (e) {
        case Error::InvalidEscape:
            return "InvalidEscape";
        case Error::InvalidKeyword:
            return "InvalidKeyword";
        case Error::InvalidNumber:
            return "InvalidNumber";
        case Error::NestingLimitReached:
            return "NestingLimitReached";
        case Error::TrailingGarbage:
            return "TrailingGarbage";
        case Error::UnexpectedCharacter:
            return "UnexpectedCharacter";
        case Error::UnexpectedControlCharacter:
            return "UnexpectedControlCharacter";
        case Error::UnexpectedEof:
            return "UnexpectedEof";
        case Error::UnpairedSurrogate:
            return "UnpairedSurrogate";
    }

    return "Unknown error";
}

struct Null {
    constexpr bool operator==(Null const &) const = default;
};

struct Array;
struct Object;

using Value = std::variant<Null, bool, std::string, std::int64_t, double, Array, Object>;

struct Array {
    std::vector<Value> values;
    inline bool operator==(Array const &) const;
};

struct Object {
    std::vector<std::pair<std::string, Value>> values;
    inline bool operator==(Object const &) const;

    [[nodiscard]] constexpr Value const &at(std::string_view key) const {
        auto it = std::ranges::find(values, key, &decltype(values)::value_type::first);
        assert(it != values.end());
        return it->second;
    }

    [[nodiscard]] constexpr decltype(values)::const_iterator find(std::string_view key) const {
        return std::ranges::find(values, key, &decltype(values)::value_type::first);
    }

    [[nodiscard]] constexpr bool contains(std::string_view key) const { return find(key) != values.end(); }
};

// TODO(robinlinden): Clang 17 and 18 crash if these are = default. Clang 19 is fine.
inline bool Array::operator==(Array const &v) const {
    return values == v.values;
}

inline bool Object::operator==(Object const &v) const {
    return values == v.values;
}

// TODO(robinlinden): Make things more constexpr once we've dropped libc++ 17, 18.
// https://www.json.org/json-en.html
class Parser {
public:
    explicit constexpr Parser(std::string_view json) : json_{json} {}

    tl::expected<Value, Error> parse() {
        static constexpr auto kRecursionLimit = 257;
        auto v = parse_value(kRecursionLimit);
        if (!v) {
            return v;
        }

        skip_whitespace();
        if (!is_eof()) {
            return tl::unexpected{Error::TrailingGarbage};
        }

        return v;
    }

private:
    std::string_view json_;
    std::size_t pos_{0};

    constexpr bool is_eof() const { return pos_ >= json_.size(); }

    constexpr bool is_whitespace(char c) const {
        switch (c) {
            case 0x09: // '\t'
            case 0x0A: // '\n'
            case 0x0D: // '\r'
            case 0x20: // ' '
                return true;
            default:
                return false;
        }
    }

    static constexpr bool is_control(unsigned char c) { return c < 0x20; }

    constexpr bool is_whitespace(std::optional<char> c) const { return c && is_whitespace(*c); }

    constexpr std::optional<char> peek() const {
        if (is_eof()) {
            return std::nullopt;
        }

        return json_[pos_];
    }

    [[nodiscard]] constexpr std::optional<char> consume() {
        if (is_eof()) {
            return std::nullopt;
        }

        return json_[pos_++];
    }

    constexpr void skip_whitespace() {
        while (!is_eof() && is_whitespace(peek())) {
            std::ignore = consume();
        }
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    tl::expected<Value, Error> parse_value(int recursion_limit) {
        if (recursion_limit <= 0) {
            return tl::unexpected{Error::NestingLimitReached};
        }

        skip_whitespace();
        auto c = peek();
        if (!c) {
            return tl::unexpected{Error::UnexpectedEof};
        }

        if (*c == '-' || (*c >= '0' && *c <= '9')) {
            return parse_number();
        }

        switch (*c) {
            case '"':
                return parse_string();
            case 't':
                return parse_true();
            case 'f':
                return parse_false();
            case 'n':
                return parse_null();
            case '[':
                return parse_array(recursion_limit);
            case '{':
                return parse_object(recursion_limit);
            default:
                return tl::unexpected{Error::UnexpectedCharacter};
        }
    }

    tl::expected<Value, Error> parse_number() {
        std::string number;
        if (auto c = peek(); c == '-') {
            number.push_back('-');
            std::ignore = consume();
        }

        if (auto c = peek(); c == '0') {
            number.push_back('0');
            std::ignore = consume();
        } else if (c >= '1' && c <= '9') {
            assert(c.has_value()); // clang-tidy 19 needs some help here.
            number.push_back(*c);
            std::ignore = consume();

            for (c = peek(); c >= '0' && c <= '9'; c = peek()) {
                assert(c.has_value()); // clang-tidy 19 needs some help here.
                number.push_back(*c);
                std::ignore = consume();
            }
        } else {
            return tl::unexpected{Error::UnexpectedCharacter};
        }

        bool is_floating_point = false;
        if (peek() == '.') {
            number.push_back('.');
            std::ignore = consume();
            is_floating_point = true;

            auto c = peek();
            if (!c) {
                return tl::unexpected{Error::UnexpectedEof};
            }

            if (*c < '0' || *c > '9') {
                return tl::unexpected{Error::UnexpectedCharacter};
            }

            number.push_back(*c);
            std::ignore = consume();

            while ((c = peek())) {
                if (*c >= '0' && *c <= '9') {
                    number.push_back(*c);
                    std::ignore = consume();
                    continue;
                }

                break;
            }
        }

        if (auto c = peek(); c == 'e' || c == 'E') {
            number.push_back(*c);
            std::ignore = consume();
            is_floating_point = true;

            if (c = peek(); c == '+' || c == '-') {
                number.push_back(*c);
                std::ignore = consume();
            }

            c = peek();
            if (!c) {
                return tl::unexpected{Error::UnexpectedEof};
            }

            if (*c < '0' || *c > '9') {
                return tl::unexpected{Error::UnexpectedCharacter};
            }

            number.push_back(*c);
            std::ignore = consume();

            while ((c = peek())) {
                if (*c >= '0' && *c <= '9') {
                    number.push_back(*c);
                    std::ignore = consume();
                    continue;
                }

                break;
            }
        }

        if (!is_floating_point) {
            std::int64_t value{};
            if (auto [p, ec] = std::from_chars(number.data(), number.data() + number.size(), value);
                    ec != std::errc{} || p != number.data() + number.size()) {
                return tl::unexpected{Error::InvalidNumber};
            }

            return Value{value};
        }

        double value{};
        if (auto [p, ec] = util::from_chars(number.data(), number.data() + number.size(), value);
                ec != std::errc{} || p != number.data() + number.size()) {
            return tl::unexpected{Error::InvalidNumber};
        }

        return Value{value};
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    tl::expected<Value, Error> parse_object(int recursion_limit) {
        std::ignore = consume(); // '{'
        skip_whitespace();

        if (peek() == '}') {
            std::ignore = consume();
            return Object{};
        }

        Object object;
        while (true) {
            skip_whitespace();

            auto key = parse_string();
            if (!key) {
                return key;
            }

            skip_whitespace();
            if (auto c = consume(); c != ':') {
                return tl::unexpected{c.has_value() ? Error::UnexpectedCharacter : Error::UnexpectedEof};
            }

            auto value = parse_value(recursion_limit - 1);
            if (!value) {
                return value;
            }

            object.values.emplace_back(std::get<std::string>(*std::move(key)), *std::move(value));
            skip_whitespace();

            auto c = peek();
            if (!c) {
                return tl::unexpected{Error::UnexpectedEof};
            }

            if (*c == ',') {
                std::ignore = consume();
                continue;
            }

            if (*c == '}') {
                std::ignore = consume();
                return object;
            }

            return tl::unexpected{Error::UnexpectedCharacter};
        }
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    tl::expected<Value, Error> parse_array(int recursion_limit) {
        std::ignore = consume(); // '['
        skip_whitespace();

        if (peek() == ']') {
            std::ignore = consume();
            return Array{};
        }

        Array array;
        while (true) {
            auto v = parse_value(recursion_limit - 1);
            if (!v) {
                return v;
            }

            array.values.push_back(*std::move(v));
            skip_whitespace();

            auto c = peek();
            if (!c) {
                return tl::unexpected{Error::UnexpectedEof};
            }

            if (*c == ',') {
                std::ignore = consume();
                continue;
            }

            if (*c == ']') {
                std::ignore = consume();
                return array;
            }

            return tl::unexpected{Error::UnexpectedCharacter};
        }
    }

    tl::expected<Value, Error> parse_true() {
        std::ignore = consume(); // 't'
        auto r = consume();
        auto u = consume();
        auto e = consume();
        if (r != 'r' || u != 'u' || e != 'e') {
            return tl::unexpected{Error::InvalidKeyword};
        }

        return Value{true};
    }

    tl::expected<Value, Error> parse_false() {
        std::ignore = consume(); // 'f'
        auto a = consume();
        auto l = consume();
        auto s = consume();
        auto e = consume();
        if (a != 'a' || l != 'l' || s != 's' || e != 'e') {
            return tl::unexpected{Error::InvalidKeyword};
        }

        return Value{false};
    }

    tl::expected<Value, Error> parse_null() {
        std::ignore = consume(); // 'n'
        auto u = consume();
        auto l1 = consume();
        auto l2 = consume();
        if (u != 'u' || l1 != 'l' || l2 != 'l') {
            return tl::unexpected{Error::InvalidKeyword};
        }

        return Value{Null{}};
    }

    tl::expected<Value, Error> parse_string() {
        std::string value;
        if (consume() != '"') {
            if (is_eof()) {
                return tl::unexpected{Error::UnexpectedEof};
            }

            return tl::unexpected{Error::UnexpectedCharacter};
        }

        while (auto c = consume()) {
            if (*c == '"') {
                return value;
            }

            if (is_control(*c)) {
                return tl::unexpected{Error::UnexpectedControlCharacter};
            }

            if (*c == '\\') {
                auto escaped = consume();
                if (!escaped) {
                    return tl::unexpected{Error::UnexpectedEof};
                }

                switch (*escaped) {
                    case '"':
                        value.push_back('"');
                        break;
                    case '\\':
                        value.push_back('\\');
                        break;
                    case '/':
                        value.push_back('/');
                        break;
                    case 'b':
                        value.push_back('\b');
                        break;
                    case 'f':
                        value.push_back('\f');
                        break;
                    case 'n':
                        value.push_back('\n');
                        break;
                    case 'r':
                        value.push_back('\r');
                        break;
                    case 't':
                        value.push_back('\t');
                        break;
                    case 'u': {
                        auto code_unit = parse_utf16_escaped_hex();
                        if (!code_unit) {
                            return code_unit;
                        }

                        if (unicode::is_high_surrogate(*code_unit)) {
                            if (consume() != '\\' || consume() != 'u') {
                                return tl::unexpected{Error::UnpairedSurrogate};
                            }

                            auto low_surrogate = parse_utf16_escaped_hex();
                            if (!low_surrogate) {
                                return low_surrogate;
                            }

                            if (!unicode::is_low_surrogate(*low_surrogate)) {
                                return tl::unexpected{Error::UnpairedSurrogate};
                            }

                            auto code_point = unicode::utf16_surrogate_pair_to_code_point(*code_unit, *low_surrogate);
                            // We always pass a high and low surrogate, so this should always succeed.
                            assert(code_point.has_value());

                            auto utf8 = unicode::to_utf8(*code_point);
                            // The only error-checking in to_utf8 is to make sure
                            // that the code point isn't too large. Not possible
                            // when there are only 4 digits.
                            assert(!utf8.empty());

                            value += utf8;
                            break;
                        }

                        auto utf8 = unicode::utf16_to_utf8(*code_unit);
                        if (!utf8) {
                            return tl::unexpected{Error::InvalidEscape};
                        }

                        value += *utf8;
                        break;
                    }
                    default:
                        return tl::unexpected{Error::UnexpectedCharacter};
                }

                continue;
            }

            value.push_back(*c);
        }

        return tl::unexpected{Error::UnexpectedEof};
    }

    // This *only* parses the 4 hex digits after the \u.
    tl::expected<std::uint16_t, Error> parse_utf16_escaped_hex() {
        std::string hex;
        for (int i = 0; i < 4; ++i) {
            auto hex_digit = consume();
            if (!hex_digit) {
                return tl::unexpected{Error::UnexpectedEof};
            }

            if (!util::is_hex_digit(*hex_digit)) {
                return tl::unexpected{Error::InvalidEscape};
            }

            hex.push_back(*hex_digit);
        }

        std::uint16_t code_unit{};
        [[maybe_unused]] auto [p, ec] = std::from_chars(hex.data(), hex.data() + hex.size(), code_unit, 16);
        // We fail earlier if the input isn't 4 hex digits, so this will always work.
        assert(ec == std::errc{} && p == hex.data() + hex.size());

        return code_unit;
    }
};

inline tl::expected<Value, Error> parse(std::string_view json) {
    return Parser{json}.parse();
}

} // namespace json

#endif
