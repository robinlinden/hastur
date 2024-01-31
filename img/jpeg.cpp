// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/jpeg.h"

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

namespace img {
namespace {

template<typename T>
[[nodiscard]] bool read_be(std::istream &is, T &value) {
    if (!is.read(reinterpret_cast<char *>(&value), sizeof(value))) {
        return false;
    }

    if constexpr (sizeof(T) > 1 && std::endian::native != std::endian::big) {
        value = std::byteswap(value);
    }

    return true;
};

struct AspectRatio {
    std::uint16_t horizontal{};
    std::uint16_t vertical{};
};

struct DotsPerInch {
    std::uint16_t horizontal{};
    std::uint16_t vertical{};
};

struct DotsPerCm {
    std::uint16_t horizontal{};
    std::uint16_t vertical{};
};

struct StartOfImage {
    static constexpr std::uint16_t kMarker = 0xFFD8;
};

struct App0Jfif {
    static constexpr std::uint16_t kMarker = 0xFFE0;

    using Density = std::variant<AspectRatio, DotsPerInch, DotsPerCm>;
    Density density{}; // units, Xdensity, Ydensity
    std::uint8_t thumbnail_x{}; // Xthumbnail
    std::uint8_t thumbnail_y{}; // Ythumbnail
    std::vector<unsigned char> thumbnail_rgb{}; // (RGB)n

    static std::optional<App0Jfif> parse(std::istream &is) {
        // TODO(robinlinden): Verify length?
        std::uint16_t length{};
        if (!read_be(is, length)) {
            return {};
        }

        std::string identifier{};
        identifier.resize(5);
        if (!is.read(identifier.data(), identifier.size()) || identifier != "JFIF\0"sv) {
            return {};
        }

        std::uint16_t version{};
        if (!read_be(is, version) || version != 0x0102) {
            return {};
        }

        std::uint8_t units{};
        if (!read_be(is, units) || units > 2) {
            return {};
        }

        std::uint16_t x_density{};
        if (!read_be(is, x_density) || x_density == 0) {
            return {};
        }

        std::uint16_t y_density{};
        if (!read_be(is, y_density) || y_density == 0) {
            return {};
        }

        std::uint8_t x_thumbnail{};
        if (!read_be(is, x_thumbnail)) {
            return {};
        }

        std::uint8_t y_thumbnail{};
        if (!read_be(is, y_thumbnail)) {
            return {};
        }

        std::vector<unsigned char> thumbnail_rgb{};
        thumbnail_rgb.resize(std::size_t{x_thumbnail} * y_thumbnail * 3);
        if (!is.read(reinterpret_cast<char *>(thumbnail_rgb.data()), thumbnail_rgb.size())) {
            return {};
        }

        static constexpr auto kToDensity = [](std::uint8_t unit, std::uint16_t x, std::uint16_t y) -> Density {
            assert(unit <= 2);
            switch (unit) {
                case 0:
                    return AspectRatio{x, y};
                case 1:
                    return DotsPerInch{x, y};
                case 2:
                default:
                    return DotsPerCm{x, y};
            }
        };

        return App0Jfif{
                kToDensity(units, x_density, y_density),
                x_thumbnail,
                y_thumbnail,
                std::move(thumbnail_rgb),
        };
    }
};

} // namespace

std::optional<Jpeg> Jpeg::thumbnail_from(std::istream &is) {
    std::uint16_t marker{};
    if (!read_be(is, marker) || marker != StartOfImage::kMarker) {
        return {};
    }

    if (!read_be(is, marker) || marker != App0Jfif::kMarker) {
        return {};
    }

    auto app0 = App0Jfif::parse(is);
    if (!app0 || app0->thumbnail_x == 0 || app0->thumbnail_y == 0) {
        return {};
    }

    auto to_rgba = [](std::vector<unsigned char> const &rgb) {
        assert(rgb.size() % 3 == 0);
        std::vector<unsigned char> rgba{};
        rgba.reserve(rgb.size() / 3 * 4);
        for (std::size_t i = 0; i < rgb.size(); i += 3) {
            rgba.push_back(rgb[i]);
            rgba.push_back(rgb[i + 1]);
            rgba.push_back(rgb[i + 2]);
            rgba.push_back(0xFF);
        }
        return rgba;
    };

    return Jpeg{app0->thumbnail_x, app0->thumbnail_y, to_rgba(app0->thumbnail_rgb)};
}

} // namespace img
