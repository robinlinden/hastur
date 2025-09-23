// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef IMG_QOI_H_
#define IMG_QOI_H_

#include <tl/expected.hpp>

#include <cstdint>
#include <iosfwd>
#include <vector>

namespace img {

enum class QoiError : std::uint8_t {
    AbruptEof,
    InvalidMagic,
    InvalidChannels,
    InvalidColorspace,
    InvalidEndMarker,
    ImageTooLarge,
};

// Compliant with the qoi specification version 1.0, 2022-01-05.
// https://qoiformat.org/
class Qoi {
public:
    static tl::expected<Qoi, QoiError> from(std::istream &&is) { return from(is); }
    static tl::expected<Qoi, QoiError> from(std::istream &is);

    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<unsigned char> bytes;

    [[nodiscard]] bool operator==(Qoi const &) const = default;
};

} // namespace img

#endif
