// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <vector>

namespace img {

class JpegTurbo {
public:
    static std::optional<JpegTurbo> decode(std::istream &&is) { return decode(is); }
    static std::optional<JpegTurbo> decode(std::istream &);

    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<unsigned char> bytes{};

    [[nodiscard]] bool operator==(JpegTurbo const &) const = default;
};

} // namespace img
