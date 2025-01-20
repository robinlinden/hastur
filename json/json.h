// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JSON_JSON_H_
#define JSON_JSON_H_

#include "unicode/util.h"
#include "util/from_chars.h"

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

    std::optional<Value> parse() {
        static constexpr auto kRecursionLimit = 257;
        auto v = parse_value(kRecursionLimit);
        skip_whitespace();

        if (!is_eof()) {
            return std::nullopt;
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
    std::optional<Value> parse_value(int recursion_limit) {
        if (recursion_limit <= 0) {
            return std::nullopt;
        }

        skip_whitespace();
        auto c = peek();
        if (!c) {
            return std::nullopt;
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
                return std::nullopt;
        }
    }

    std::optional<Value> parse_number() {
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
            return std::nullopt;
        }

        bool is_floating_point = false;
        if (peek() == '.') {
            number.push_back('.');
            std::ignore = consume();
            is_floating_point = true;

            auto c = peek();
            if (!c || *c < '0' || *c > '9') {
                return std::nullopt;
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

            if (c = peek(); !c || *c < '0' || *c > '9') {
                return std::nullopt;
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
                return std::nullopt;
            }

            return Value{value};
        }

        double value{};
        if (auto [p, ec] = util::from_chars(number.data(), number.data() + number.size(), value);
                ec != std::errc{} || p != number.data() + number.size()) {
            return std::nullopt;
        }

        return Value{value};
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    std::optional<Value> parse_object(int recursion_limit) {
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
                return std::nullopt;
            }

            skip_whitespace();
            if (consume() != ':') {
                return std::nullopt;
            }

            auto value = parse_value(recursion_limit - 1);
            if (!value) {
                return std::nullopt;
            }

            object.values.emplace_back(std::get<std::string>(*std::move(key)), *std::move(value));
            skip_whitespace();

            auto c = peek();
            if (!c) {
                return std::nullopt;
            }

            if (*c == ',') {
                std::ignore = consume();
                continue;
            }

            if (*c == '}') {
                std::ignore = consume();
                return object;
            }

            return std::nullopt;
        }
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    std::optional<Value> parse_array(int recursion_limit) {
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
                return std::nullopt;
            }

            array.values.push_back(*std::move(v));
            skip_whitespace();

            auto c = peek();
            if (!c) {
                return std::nullopt;
            }

            if (*c == ',') {
                std::ignore = consume();
                continue;
            }

            if (*c == ']') {
                std::ignore = consume();
                return array;
            }

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
        if (consume() != '"') {
            return std::nullopt;
        }

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
};

inline std::optional<Value> parse(std::string_view json) {
    return Parser{json}.parse();
}

} // namespace json

#endif
