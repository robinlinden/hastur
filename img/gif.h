// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef IMG_GIF_H_
#define IMG_GIF_H_

#include <cstdint>
#include <iosfwd>
#include <optional>

namespace img {

class Gif {
public:
    enum class Version {
        Gif87a,
        Gif89a,
    };

    static std::optional<Gif> from(std::istream &&is) { return from(is); }
    static std::optional<Gif> from(std::istream &is);

    Version version{};
    std::uint32_t width{};
    std::uint32_t height{};

    [[nodiscard]] bool operator==(Gif const &) const = default;
};

} // namespace img

#endif
