// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/qoi.h"

#include <tl/expected.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>
#include <utility>
#include <vector>

namespace img {
namespace {

// 8-bit tags.
constexpr std::uint8_t kQoiOpRgb = 0b1111'1110;
constexpr std::uint8_t kQoiOpRgba = 0b1111'1111;

// 2-bit tags.
constexpr std::uint8_t kQoiOpIndex = 0b0000'0000;
constexpr std::uint8_t kQoiOpDiff = 0b0100'0000;
constexpr std::uint8_t kQoiOpLuma = 0b1000'0000;
constexpr std::uint8_t kQoiOpRun = 0b1100'0000;

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

    // We don't support images larger than 400 million pixels (~1.5GiB).
    // This matches the implementation at https://github.com/phoboslab/qoi
    static constexpr std::size_t kMaxPixelCount{400'000'000};
    if ((width > 0) && (height > kMaxPixelCount / width)) {
        return tl::unexpected{QoiError::ImageTooLarge};
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
        } else if (chunk == kQoiOpRgba) {
            if (!is.read(reinterpret_cast<char *>(&previous_pixel), 4)) {
                return tl::unexpected{QoiError::AbruptEof};
            }
        } else if (short_tag == kQoiOpIndex) {
            previous_pixel = seen_pixels[short_value];
        } else if (short_tag == kQoiOpDiff) {
            // Stored with a bias of 2.
            auto const db = (short_value & 0b11) - 2;
            auto const dg = ((short_value >> 2) & 0b11) - 2;
            auto const dr = ((short_value >> 4) & 0b11) - 2;
            previous_pixel.b = static_cast<std::uint8_t>(previous_pixel.b + db);
            previous_pixel.g = static_cast<std::uint8_t>(previous_pixel.g + dg);
            previous_pixel.r = static_cast<std::uint8_t>(previous_pixel.r + dr);
        } else if (short_tag == kQoiOpLuma) {
            std::uint8_t extra_data{};
            if (!is.read(reinterpret_cast<char *>(&extra_data), sizeof(extra_data))) {
                return tl::unexpected{QoiError::AbruptEof};
            }

            static constexpr auto kGreenBias = -32;
            static constexpr auto kRedBlueBias = -8;
            auto const diff_green = short_value + kGreenBias;
            auto const diff_blue = (extra_data & 0b1111) + diff_green + kRedBlueBias;
            auto const diff_red = ((extra_data >> 4) & 0b1111) + diff_green + kRedBlueBias;
            previous_pixel.b = static_cast<std::uint8_t>(previous_pixel.b + diff_blue);
            previous_pixel.g = static_cast<std::uint8_t>(previous_pixel.g + diff_green);
            previous_pixel.r = static_cast<std::uint8_t>(previous_pixel.r + diff_red);
        } else if (short_tag == kQoiOpRun) {
            // Stored with a bias of -1.
            auto const run_length = short_value + 1;
            for (int i = 0; i < run_length; ++i) {
                pixels.push_back(previous_pixel.r);
                pixels.push_back(previous_pixel.g);
                pixels.push_back(previous_pixel.b);
                pixels.push_back(previous_pixel.a);
            }
            continue;
        }

        pixels.push_back(previous_pixel.r);
        pixels.push_back(previous_pixel.g);
        pixels.push_back(previous_pixel.b);
        pixels.push_back(previous_pixel.a);
        seen_pixels[seen_pixels_index(previous_pixel)] = previous_pixel;
    }

    // The byte stream's end is marked with 7 0x00 bytes followed by a single
    // 0x01 byte.
    std::array<std::uint8_t, 8> footer{};
    if (!is.read(reinterpret_cast<char *>(footer.data()), footer.size())) {
        return tl::unexpected{QoiError::AbruptEof};
    }

    if (footer != decltype(footer){0, 0, 0, 0, 0, 0, 0, 1}) {
        return tl::unexpected{QoiError::InvalidEndMarker};
    }

    return Qoi{
            .width = width,
            .height = height,
            .bytes = std::move(pixels),
    };
}

} // namespace img
