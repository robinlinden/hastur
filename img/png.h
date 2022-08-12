// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef IMG_PNG_H_
#define IMG_PNG_H_

#include <cstdint>
#include <iosfwd>
#include <optional>

namespace img {

class Png {
public:
    static std::optional<Png> from(std::istream &is);

    std::uint32_t width{};
    std::uint32_t height{};

    [[nodiscard]] bool operator==(Png const &) const = default;
};

} // namespace img

#endif
