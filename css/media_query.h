// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS_MEDIA_QUERY_H_
#define CSS_MEDIA_QUERY_H_

#include "util/from_chars.h"
#include "util/string.h"

#include <cassert>
#include <charconv>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

namespace css {

class MediaQuery;

enum class ColorScheme : std::uint8_t {
    Light,
    Dark,
};

enum class MediaType : std::uint8_t {
    Print,
    Screen,
};

// This namespace is a workaround required when using libstdc++.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96645
namespace detail {

struct Context {
    int window_width{};
    ColorScheme color_scheme{ColorScheme::Light};
    MediaType media_type{MediaType::Screen};
};

struct And;
struct False;
struct PrefersColorScheme;
struct True;
struct Type;
struct Width;
using Query = std::variant<And, False, PrefersColorScheme, True, Type, Width>;

struct False {
    [[nodiscard]] bool operator==(False const &) const = default;
    constexpr bool evaluate(Context const &) const { return false; }
};

// https://developer.mozilla.org/en-US/docs/Web/CSS/@media/prefers-color-scheme
struct PrefersColorScheme {
    ColorScheme color_scheme{};
    [[nodiscard]] bool operator==(PrefersColorScheme const &) const = default;

    constexpr bool evaluate(Context const &ctx) const { return ctx.color_scheme == color_scheme; }
};

struct True {
    [[nodiscard]] bool operator==(True const &) const = default;
    constexpr bool evaluate(Context const &) const { return true; }
};

// https://developer.mozilla.org/en-US/docs/Web/CSS/@media#media_types
struct Type {
    MediaType type{};
    [[nodiscard]] bool operator==(Type const &) const = default;

    constexpr bool evaluate(Context const &ctx) const { return ctx.media_type == type; }
};

struct Width {
    int min{};
    int max{std::numeric_limits<int>::max()};
    [[nodiscard]] bool operator==(Width const &) const = default;

    constexpr bool evaluate(Context const &ctx) const { return min <= ctx.window_width && ctx.window_width <= max; }
};

struct And {
    // TODO(robinlinden): shared_ptr here is sad, but we stick this in
    // std::vector, so it needs to be copyable for now.
    std::shared_ptr<MediaQuery> lhs{};
    std::shared_ptr<MediaQuery> rhs{};
    [[nodiscard]] inline bool operator==(And const &other) const;

    inline bool evaluate(Context const &) const;
};

} // namespace detail

class MediaQuery {
public:
    using And = detail::And;
    using Context = detail::Context;
    using False = detail::False;
    using PrefersColorScheme = detail::PrefersColorScheme;
    using True = detail::True;
    using Type = detail::Type;
    using Width = detail::Width;

    using Query = detail::Query;
    Query query{};
    [[nodiscard]] bool operator==(MediaQuery const &) const = default;

    // https://drafts.csswg.org/mediaqueries/#mq-syntax
    // NOLINTNEXTLINE(misc-no-recursion)
    static std::optional<MediaQuery> parse(std::string_view s) {
        if (auto and_pos = s.find(" and "); and_pos != std::string_view::npos) {
            auto lhs = parse(s.substr(0, and_pos));
            if (!lhs) {
                return std::nullopt;
            }

            auto rhs = parse(s.substr(and_pos + 5));
            if (!rhs) {
                return std::nullopt;
            }

            return MediaQuery{And{
                    .lhs = std::make_shared<MediaQuery>(std::move(*lhs)),
                    .rhs = std::make_shared<MediaQuery>(std::move(*rhs)),
            }};
        }

        if (s == "all" || s == "only all") {
            return MediaQuery{True{}};
        }

        if (s == "print" || s == "only print") {
            return MediaQuery{Type{.type = MediaType::Print}};
        }

        if (s == "screen" || s == "only screen") {
            return MediaQuery{Type{.type = MediaType::Screen}};
        }

        if (!(s.starts_with('(') && s.ends_with(')'))) {
            return std::nullopt;
        }

        s.remove_prefix(1);
        s.remove_suffix(1);

        // And we only handle mf-plain right now.
        s = util::trim(s);
        auto feature_name_end = s.find_first_of(" :");
        if (feature_name_end == std::string_view::npos) {
            return std::nullopt;
        }
        auto feature_name = s.substr(0, feature_name_end);
        s.remove_prefix(feature_name_end);

        auto value_start = s.find_first_not_of(" :");
        if (value_start == std::string_view::npos) {
            return std::nullopt;
        }
        auto value_str = s.substr(value_start);

        if (feature_name == "width" || feature_name == "min-width" || feature_name == "max-width") {
            return parse_width(feature_name, value_str);
        }

        if (feature_name == "prefers-color-scheme") {
            if (value_str == "light") {
                return MediaQuery{PrefersColorScheme{.color_scheme = ColorScheme::Light}};
            }

            if (value_str == "dark") {
                return MediaQuery{PrefersColorScheme{.color_scheme = ColorScheme::Dark}};
            }

            return std::nullopt;
        }

        return std::nullopt;
    }

    constexpr bool evaluate(Context const &ctx) const {
        return std::visit([&](auto const &q) { return q.evaluate(ctx); }, query);
    }

private:
    static std::optional<MediaQuery> parse_width(std::string_view feature_name, std::string_view value_str) {
        float value{};
        auto value_parse_res = util::from_chars(value_str.data(), value_str.data() + value_str.size(), value);
        if (value_parse_res.ec != std::errc{}) {
            return std::nullopt;
        }

        auto value_unit = value_str.substr(std::distance(value_str.data(), value_parse_res.ptr));
        if (value != 0 && value_unit.empty()) {
            return std::nullopt;
        }

        // ...and we only handle px as the unit.
        if (value != 0 && value_unit != "px") {
            return std::nullopt;
        }

        if (feature_name == "min-width") {
            return MediaQuery{Width{.min = static_cast<int>(value)}};
        }

        if (feature_name == "max-width") {
            return MediaQuery{Width{.max = static_cast<int>(value)}};
        }

        assert(feature_name == "width");
        return MediaQuery{Width{.min = static_cast<int>(value), .max = static_cast<int>(value)}};
    }
};

inline bool detail::And::operator==(And const &other) const {
    assert(lhs && rhs);
    return *lhs == *other.lhs && *rhs == *other.rhs;
}

inline bool detail::And::evaluate(Context const &ctx) const {
    assert(lhs && rhs);
    return lhs->evaluate(ctx) && rhs->evaluate(ctx);
}

inline std::string to_string(MediaQuery::Width const &width) {
    return std::to_string(width.min) + " <= width <= " + std::to_string(width.max);
}

constexpr std::string to_string(MediaQuery::False const &) {
    return "false";
}

constexpr std::string to_string(MediaQuery::True const &) {
    return "true";
}

constexpr std::string to_string(MediaQuery::Type const &q) {
    return q.type == MediaType::Print ? "print" : "screen";
}

constexpr std::string to_string(MediaQuery::PrefersColorScheme const &q) {
    return q.color_scheme == ColorScheme::Light ? "prefers-color-scheme: light" : "prefers-color-scheme: dark";
}

constexpr std::string to_string(MediaQuery::And const &);

constexpr std::string to_string(MediaQuery const &query) {
    return std::visit([](auto const &q) { return to_string(q); }, query.query);
}

constexpr std::string to_string(MediaQuery::And const &q) {
    return to_string(*q.lhs) + " and " + to_string(*q.rhs);
}

} // namespace css

#endif
