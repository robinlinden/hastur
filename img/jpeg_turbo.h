// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef IMG_JPEG_TURBO_H_
#define IMG_JPEG_TURBO_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <iterator>
#include <optional>
#include <span>
#include <vector>

namespace img {

class JpegTurbo {
public:
    static std::optional<JpegTurbo> from(std::span<std::byte const>);

    static std::optional<JpegTurbo> from(std::istream &is) {
        std::vector<std::byte> bytes{};
        std::ranges::transform(std::istreambuf_iterator<char>{is},
                std::istreambuf_iterator<char>{},
                std::back_inserter(bytes),
                [](char c) { return static_cast<std::byte>(c); });

        return from(bytes);
    }

    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<unsigned char> bytes{};

    [[nodiscard]] bool operator==(JpegTurbo const &) const = default;
};

} // namespace img

#endif
