// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef IMG_JPEG_H_
#define IMG_JPEG_H_

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <vector>

namespace img {

// https://www.w3.org/Graphics/JPEG/jfif3.pdf
// https://www.w3.org/Graphics/JPEG/itu-t81.pdf
class Jpeg {
public:
    static std::optional<Jpeg> thumbnail_from(std::istream &&is) { return thumbnail_from(is); }
    static std::optional<Jpeg> thumbnail_from(std::istream &);

    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<unsigned char> bytes{};

    [[nodiscard]] bool operator==(Jpeg const &) const = default;
};

} // namespace img

#endif
