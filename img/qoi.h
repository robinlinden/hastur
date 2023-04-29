// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef IMG_QOI_H_
#define IMG_QOI_H_

#include <tl/expected.hpp>

#include <cstdint>
#include <iosfwd>
#include <vector>

namespace img {

enum class QoiError {
    AbruptEof,
    InvalidMagic,
    InvalidChannels,
    InvalidColorspace,
    UnhandledChunk,
};

class Qoi {
public:
    static tl::expected<Qoi, QoiError> from(std::istream &&is) { return from(is); }
    static tl::expected<Qoi, QoiError> from(std::istream &is);

    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<unsigned char> bytes{};

    [[nodiscard]] bool operator==(Qoi const &) const = default;
};

} // namespace img

#endif
