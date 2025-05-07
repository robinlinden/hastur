// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS_MEDIA_QUERY_H_
#define CSS_MEDIA_QUERY_H_

#include "util/from_chars.h"
#include "util/string.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

namespace css {

class MediaQuery;

enum class ColorScheme : std::uint8_t {
    Light,
    Dark,
};

enum class Hover : std::uint8_t {
    None,
    Hover,
};

enum class MediaType : std::uint8_t {
    Print,
    Screen,
};

enum class Orientation : std::uint8_t {
    Landscape,
    Portrait,
};

enum class ReduceMotion : std::uint8_t {
    NoPreference,
    Reduce,
};

// This namespace is a workaround required when using libstdc++.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96645
namespace detail {

struct Context {
    int window_width{};
    int window_height{};
    ColorScheme color_scheme{ColorScheme::Light};
    Hover hover{Hover::None};
    MediaType media_type{MediaType::Screen};
    Orientation orientation{window_height >= window_width ? Orientation::Portrait : Orientation::Landscape};
    ReduceMotion reduce_motion{ReduceMotion::NoPreference};
};

struct And;
struct False;
struct Height;
struct HoverType;
struct IsInOrientation;
struct PrefersColorScheme;
struct PrefersReducedMotion;
struct True;
struct Type;
struct Width;
using Query = std::variant<And,
        False,
        Height,
        IsInOrientation,
        PrefersColorScheme,
        PrefersReducedMotion,
        HoverType,
        True,
        Type,
        Width //
        >;

struct False {
    [[nodiscard]] bool operator==(False const &) const = default;
    constexpr bool evaluate(Context const &) const { return false; }
};

// https://developer.mozilla.org/en-US/docs/Web/CSS/@media/hover
struct HoverType {
    Hover hover{};
    [[nodiscard]] bool operator==(HoverType const &) const = default;

    constexpr bool evaluate(Context const &ctx) const { return ctx.hover == hover; }
};

// https://developer.mozilla.org/en-US/docs/Web/CSS/@media/height
struct Height {
    int min{};
    int max{std::numeric_limits<int>::max()};
    [[nodiscard]] bool operator==(Height const &) const = default;

    constexpr bool evaluate(Context const &ctx) const { return min <= ctx.window_height && ctx.window_height <= max; }
};

// https://developer.mozilla.org/en-US/docs/Web/CSS/@media/orientation
struct IsInOrientation {
    Orientation orientation{};
    [[nodiscard]] bool operator==(IsInOrientation const &) const = default;

    constexpr bool evaluate(Context const &ctx) const { return ctx.orientation == orientation; }
};

// https://developer.mozilla.org/en-US/docs/Web/CSS/@media/prefers-color-scheme
struct PrefersColorScheme {
    ColorScheme color_scheme{};
    [[nodiscard]] bool operator==(PrefersColorScheme const &) const = default;

    constexpr bool evaluate(Context const &ctx) const { return ctx.color_scheme == color_scheme; }
};

// https://developer.mozilla.org/en-US/docs/Web/CSS/@media/prefers-reduced-motion
struct PrefersReducedMotion {
    [[nodiscard]] bool operator==(PrefersReducedMotion const &) const = default;

    constexpr bool evaluate(Context const &ctx) const { return ctx.reduce_motion == ReduceMotion::Reduce; }
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
    std::vector<MediaQuery> queries;
    [[nodiscard]] bool operator==(And const &other) const = default;

    inline bool evaluate(Context const &) const;
};

} // namespace detail

class MediaQuery {
public:
    using And = detail::And;
    using Context = detail::Context;
    using False = detail::False;
    using Height = detail::Height;
    using HoverType = detail::HoverType;
    using IsInOrientation = detail::IsInOrientation;
    using PrefersColorScheme = detail::PrefersColorScheme;
    using PrefersReducedMotion = detail::PrefersReducedMotion;
    using True = detail::True;
    using Type = detail::Type;
    using Width = detail::Width;

    using Query = detail::Query;
    Query query{};
    [[nodiscard]] bool operator==(MediaQuery const &) const = default;

    // https://drafts.csswg.org/mediaqueries/#mq-syntax
    static std::optional<MediaQuery> parse(std::string_view s) {
        if (s.contains(" and ")) {
            return parse_and(s);
        }

        return parse_impl(s);
    }

    constexpr bool evaluate(Context const &ctx) const {
        return std::visit([&](auto const &q) { return q.evaluate(ctx); }, query);
    }

private:
    static std::optional<MediaQuery> parse_impl(std::string_view s) {
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
            return parse_length<Width>("width", feature_name, value_str);
        }

        if (feature_name == "height" || feature_name == "min-height" || feature_name == "max-height") {
            return parse_length<Height>("height", feature_name, value_str);
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

        if (feature_name == "prefers-reduced-motion") {
            if (value_str == "reduce") {
                return MediaQuery{PrefersReducedMotion{}};
            }

            if (value_str == "no-preference") {
                return MediaQuery{False{}};
            }

            return std::nullopt;
        }

        if (feature_name == "hover") {
            if (value_str == "hover") {
                return MediaQuery{HoverType{.hover = Hover::Hover}};
            }

            if (value_str == "none") {
                return MediaQuery{HoverType{.hover = Hover::None}};
            }

            return std::nullopt;
        }

        if (feature_name == "orientation") {
            if (value_str == "landscape") {
                return MediaQuery{IsInOrientation{.orientation = Orientation::Landscape}};
            }

            if (value_str == "portrait") {
                return MediaQuery{IsInOrientation{.orientation = Orientation::Portrait}};
            }

            return std::nullopt;
        }

        return std::nullopt;
    }

    static std::optional<MediaQuery> parse_and(std::string_view s) {
        auto query_parts = util::split(s, " and ");
        assert(query_parts.size() >= 2);

        std::vector<MediaQuery> queries;
        queries.reserve(query_parts.size());
        for (auto const &part : query_parts) {
            auto query = parse_impl(part);
            if (!query) {
                return std::nullopt;
            }

            queries.push_back(std::move(*query));
        }

        return MediaQuery{And{.queries = std::move(queries)}};
    }

    template<typename T>
    static std::optional<MediaQuery> parse_length(
            std::string_view suffix, std::string_view feature_name, std::string_view value_str) {
        // Checked by the caller.
        assert(feature_name.ends_with(suffix));
        feature_name.remove_suffix(suffix.size());

        float value{};
        auto value_parse_res = util::from_chars(value_str.data(), value_str.data() + value_str.size(), value);
        if (value_parse_res.ec != std::errc{}) {
            return std::nullopt;
        }

        auto value_unit = value_str.substr(std::distance(value_str.data(), value_parse_res.ptr));
        if (value != 0 && value_unit.empty()) {
            return std::nullopt;
        }

        if (value_unit == "em" || value_unit == "rem") {
            // TODO(robinlinden): Make configurable. Needs to match the default
            // font size in the StyledNode property calulations right now.
            static constexpr int kDefaultFontSize{16};
            value *= kDefaultFontSize;
            value_unit = "px";
        }

        // ...and we only handle px as the unit.
        if (value != 0 && value_unit != "px") {
            return std::nullopt;
        }

        if (feature_name == "min-") {
            return MediaQuery{T{.min = static_cast<int>(value)}};
        }

        if (feature_name == "max-") {
            return MediaQuery{T{.max = static_cast<int>(value)}};
        }

        assert(feature_name.empty());
        return MediaQuery{T{.min = static_cast<int>(value), .max = static_cast<int>(value)}};
    }
};

inline bool detail::And::evaluate(Context const &ctx) const {
    return std::ranges::all_of(queries, [&](auto const &q) { return q.evaluate(ctx); });
}

inline std::string to_string(MediaQuery::Width const &width) {
    return std::to_string(width.min) + " <= width <= " + std::to_string(width.max);
}

inline std::string to_string(MediaQuery::Height const &height) {
    return std::to_string(height.min) + " <= height <= " + std::to_string(height.max);
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

constexpr std::string to_string(MediaQuery::PrefersReducedMotion const &) {
    return "prefers-reduced-motion: reduce";
}

constexpr std::string to_string(MediaQuery::HoverType const &q) {
    return q.hover == Hover::Hover ? "hover: hover" : "hover: none";
}

constexpr std::string to_string(MediaQuery::IsInOrientation const &q) {
    return q.orientation == Orientation::Landscape ? "orientation: landscape" : "orientation: portrait";
}

constexpr std::string to_string(MediaQuery::And const &);

constexpr std::string to_string(MediaQuery const &query) {
    return std::visit([](auto const &q) { return to_string(q); }, query.query);
}

constexpr std::string to_string(MediaQuery::And const &q) {
    assert(!q.queries.empty());
    std::string res;
    for (std::size_t i = 0; i < q.queries.size() - 1; ++i) {
        res += to_string(q.queries[i]) + " and ";
    }
    res += to_string(q.queries.back());

    return res;
}

} // namespace css

#endif
