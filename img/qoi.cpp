// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/qoi.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>
#include <vector>

namespace img {
namespace {

// 8-bit tags.
constexpr std::uint8_t kQoiOpRgb = 0b1111'1110;

// 2-bit tags.
constexpr std::uint8_t kQoiOpIndex = 0b0000'0000;

struct Px {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
    std::uint8_t a{};
};

std::size_t seen_pixels_index(Px const &px) {
    return (std::size_t{px.r} * 3 + std::size_t{px.g} * 5 + std::size_t{px.b} * 7 + std::size_t{px.a} * 11) % 64;
}

} // namespace

// https://qoiformat.org/qoi-specification.pdf
tl::expected<Qoi, QoiError> Qoi::from(std::istream &is) {
    // A QOI file consists of a 14-byte header, followed by any number of
    // data "chunks" and an 8-byte end marker.
    //
    // qoi_header {
    //     char magic[4]; // magic bytes "qoif"
    //     uint32_t width; // image width in pixels (BE)
    //     uint32_t height; // image height in pixels (BE)
    //     uint8_t channels; // 3 = RGB, 4 = RGBA
    //     uint8_t colorspace; // 0 = sRGB with linear alpha, 1 = all channels linear
    // };

    std::string magic{};
    magic.resize(4);

    if (!is.read(magic.data(), magic.size())) {
        return tl::unexpected{QoiError::AbruptEof};
    }

    if (magic != "qoif") {
        return tl::unexpected{QoiError::InvalidMagic};
    }

    std::uint32_t width{};
    if (!is.read(reinterpret_cast<char *>(&width), sizeof(width))) {
        return tl::unexpected{QoiError::AbruptEof};
    }

    std::uint32_t height{};
    if (!is.read(reinterpret_cast<char *>(&height), sizeof(height))) {
        return tl::unexpected{QoiError::AbruptEof};
    }

    static_assert((std::endian::native == std::endian::big) || (std::endian::native == std::endian::little),
            "Mixed endian is unsupported right now");
    if constexpr (std::endian::native != std::endian::big) {
        width = std::byteswap(width);
        height = std::byteswap(height);
    }

    std::uint8_t channels{};
    if (!is.read(reinterpret_cast<char *>(&channels), sizeof(channels))) {
        return tl::unexpected{QoiError::AbruptEof};
    }

    if (channels != 3 && channels != 4) {
        return tl::unexpected{QoiError::InvalidChannels};
    }

    std::uint8_t colorspace{};
    if (!is.read(reinterpret_cast<char *>(&colorspace), sizeof(colorspace))) {
        return tl::unexpected{QoiError::AbruptEof};
    }

    if (colorspace != 0 && colorspace != 1) {
        return tl::unexpected{QoiError::InvalidColorspace};
    }

    std::vector<unsigned char> pixels{};
    auto const bytes_needed = std::size_t{width} * height * 4;
    pixels.reserve(bytes_needed);

    Px previous_pixel{0, 0, 0, 255};
    std::array<Px, 64> seen_pixels{};
    while (pixels.size() != bytes_needed) {
        std::uint8_t chunk{};
        if (!is.read(reinterpret_cast<char *>(&chunk), sizeof(chunk))) {
            return tl::unexpected{QoiError::AbruptEof};
        }

        auto const short_tag = chunk & 0b1100'0000;
        auto const short_value = chunk & 0b0011'1111;

        if (chunk == kQoiOpRgb) {
            if (!is.read(reinterpret_cast<char *>(&previous_pixel), 3)) {
                return tl::unexpected{QoiError::AbruptEof};
            }
        } else if (short_tag == kQoiOpIndex) {
            previous_pixel = seen_pixels[short_value];
        } else {
            return tl::unexpected{QoiError::UnhandledChunk};
        }

        pixels.push_back(previous_pixel.r);
        pixels.push_back(previous_pixel.g);
        pixels.push_back(previous_pixel.b);
        pixels.push_back(previous_pixel.a);
        seen_pixels[seen_pixels_index(previous_pixel)] = previous_pixel;
    }

    return Qoi{
            .width = width,
            .height = height,
            .bytes = std::move(pixels),
    };
}

} // namespace img
