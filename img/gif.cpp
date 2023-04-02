// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/gif.h"

#include <istream>
#include <string>
#include <string_view>

using namespace std::literals;

namespace img {
namespace {

// 18. Logical Screen Descriptor
//
//       7 6 5 4 3 2 1 0        Field Name                    Type
//      +---------------+
//   0  |               |       Logical Screen Width          Unsigned
//      +-             -+
//   1  |               |
//      +---------------+
//   2  |               |       Logical Screen Height         Unsigned
//      +-             -+
//   3  |               |
//      +---------------+
//   4  | |     | |     |       <Packed Fields>               See below
//      +---------------+
//   5  |               |       Background Color Index        Byte
//      +---------------+
//   6  |               |       Pixel Aspect Ratio            Byte
//      +---------------+
//
//      <Packed Fields>  =      Global Color Table Flag       1 Bit
//                              Color Resolution              3 Bits
//                              Sort Flag                     1 Bit
//                              Size of Global Color Table    3 Bits
struct ScreenDescriptor {
    std::uint16_t width{};
    std::uint16_t height{};

    bool global_color_table{};
    std::uint8_t color_resolution{};
    bool sort{};
    std::uint8_t size_of_global_color_table{};

    std::uint8_t background_color_index{};
    std::uint8_t pixel_aspect_ratio{};

    static std::optional<ScreenDescriptor> from(std::istream &is) {
        ScreenDescriptor screen{};

        if (!is.read(reinterpret_cast<char *>(&screen.width), sizeof(screen.width))) {
            return std::nullopt;
        }

        if (!is.read(reinterpret_cast<char *>(&screen.height), sizeof(screen.height))) {
            return std::nullopt;
        }

        std::uint8_t packed_fields{};
        if (!is.read(reinterpret_cast<char *>(&packed_fields), sizeof(packed_fields))) {
            return std::nullopt;
        }

        screen.global_color_table = packed_fields & 0b1000'0000;
        screen.color_resolution = (packed_fields & 0b0111'0000) >> 4;
        screen.sort = packed_fields & 0b0000'1000;
        screen.size_of_global_color_table = packed_fields & 0b0000'0111;

        if (!is.read(reinterpret_cast<char *>(&screen.background_color_index), sizeof(screen.background_color_index))) {
            return std::nullopt;
        }

        if (!is.read(reinterpret_cast<char *>(&screen.pixel_aspect_ratio), sizeof(screen.pixel_aspect_ratio))) {
            return std::nullopt;
        }

        return screen;
    }
};

} // namespace

// https://www.w3.org/Graphics/GIF/spec-gif87.txt
// https://www.w3.org/Graphics/GIF/spec-gif89a.txt
std::optional<Gif> Gif::from(std::istream &is) {
    // 17. Header

    // i) Signature - Identifies the GIF Data Stream. This field contains
    // the fixed value 'GIF'.

    // ii) Version - Version number used to format the data stream.
    // Identifies the minimum set of capabilities necessary to a decoder
    // to fully process the contents of the Data Stream.

    // Version Numbers as of 10 July 1990 :       "87a" - May 1987
    //                                            "89a" - July 1989
    std::string magic{};
    magic.resize(6);

    if (!is.read(magic.data(), magic.size())) {
        return std::nullopt;
    }

    Gif::Version version{};
    if (magic == "GIF87a"sv) {
        version = Gif::Version::Gif87a;
    } else if (magic == "GIF89a"sv) {
        version = Gif::Version::Gif89a;
    } else {
        return std::nullopt;
    }

    auto screen = ScreenDescriptor::from(is);
    if (!screen) {
        return std::nullopt;
    }

    return Gif{
            .version = version,
            .width = screen->width,
            .height = screen->height,
    };
}

} // namespace img
