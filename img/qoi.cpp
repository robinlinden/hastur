// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/qoi.h"

#include <bit>
#include <istream>
#include <string>

namespace img {

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

    return Qoi{
            .width = width,
            .height = height,
    };
}

} // namespace img
