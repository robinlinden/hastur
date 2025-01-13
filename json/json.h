// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JSON_JSON_H_
#define JSON_JSON_H_

#include "unicode/util.h"

#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <variant>
#include <vector>

namespace json {

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
    std::map<std::string, Value, std::less<>> values;
    inline bool operator==(Object const &) const;
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

    std::optional<Value> parse() {
        auto v = parse_value();
        skip_whitespace();

        if (!is_eof()) {
            return std::nullopt;
        }

        return v;
    }

    std::optional<Value> parse_value() {
        skip_whitespace();
        auto c = peek();
        if (!c) {
            return std::nullopt;
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
            default:
                return std::nullopt;
        }
    }

    std::optional<Value> parse_true() {
        std::ignore = consume(); // 't'
        auto r = consume();
        auto u = consume();
        auto e = consume();
        if (r != 'r' || u != 'u' || e != 'e') {
            return std::nullopt;
        }

        return Value{true};
    }

    std::optional<Value> parse_false() {
        std::ignore = consume(); // 'f'
        auto a = consume();
        auto l = consume();
        auto s = consume();
        auto e = consume();
        if (a != 'a' || l != 'l' || s != 's' || e != 'e') {
            return std::nullopt;
        }

        return Value{false};
    }

    std::optional<Value> parse_null() {
        std::ignore = consume(); // 'n'
        auto u = consume();
        auto l1 = consume();
        auto l2 = consume();
        if (u != 'u' || l1 != 'l' || l2 != 'l') {
            return std::nullopt;
        }

        return Value{Null{}};
    }

    std::optional<Value> parse_string() {
        std::string value;
        std::ignore = consume(); // '"'
        while (auto c = consume()) {
            if (*c == '"') {
                return value;
            }

            if (*c == '\\') {
                auto escaped = consume();
                if (!escaped) {
                    return std::nullopt;
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
                            return std::nullopt;
                        }

                        if (unicode::is_high_surrogate(*code_unit)) {
                            if (consume() != '\\' || consume() != 'u') {
                                return std::nullopt;
                            }

                            auto low_surrogate = parse_utf16_escaped_hex();
                            if (!low_surrogate || !unicode::is_low_surrogate(*low_surrogate)) {
                                return std::nullopt;
                            }

                            auto code_point = unicode::utf16_surrogate_pair_to_code_point(*code_unit, *low_surrogate);
                            if (!code_point) {
                                return std::nullopt;
                            }

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
                            return std::nullopt;
                        }

                        value += *utf8;
                        break;
                    }
                    default:
                        return std::nullopt;
                }

                continue;
            }

            value.push_back(*c);
        }

        return std::nullopt;
    }

    // This *only* parses the 4 hex digits after the \u.
    std::optional<std::uint16_t> parse_utf16_escaped_hex() {
        std::string hex;
        for (int i = 0; i < 4; ++i) {
            auto hex_digit = consume();
            if (!hex_digit) {
                return std::nullopt;
            }

            hex.push_back(*hex_digit);
        }

        std::uint16_t code_unit{};
        if (auto [p, ec] = std::from_chars(hex.data(), hex.data() + hex.size(), code_unit, 16);
                ec != std::errc{} || p != hex.data() + hex.size()) {
            return std::nullopt;
        }

        return code_unit;
    }

private:
    std::string_view json_;
    std::size_t pos_{0};
};

inline std::optional<Value> parse(std::string_view json) {
    return Parser{json}.parse();
}

} // namespace json

#endif
