// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/color.h"

#include "util/string.h"

#include <algorithm>
#include <map>

namespace gfx {
namespace {

struct CaseInsensitiveLess {
    using is_transparent = void;
    bool operator()(std::string_view s1, std::string_view s2) const {
        return std::ranges::lexicographical_compare(
                s1, s2, [](char c1, char c2) { return util::lowercased(c1) < util::lowercased(c2); });
    }
};

// https://developer.mozilla.org/en-US/docs/Web/CSS/named-color#list_of_all_color_keywords
// NOLINTNEXTLINE(cert-err58-cpp)
std::map<std::string_view, gfx::Color, CaseInsensitiveLess> const named_colors{
        // System colors.
        // https://developer.mozilla.org/en-US/docs/Web/CSS/color_value#system_colors
        // TODO(robinlinden): Move these elsewhere and actually grab them from the system.
        //   Right now these are based on what the CSS Color 4 spec says the traditional colors are.
        //   See: https://www.w3.org/TR/css-color-4/#css-system-colors
        // TODO(robinlinden): More system colors. Right now, we only have the most common ones.
        {"canvas", gfx::Color::from_rgb(0xff'ff'ff)}, // white
        {"canvastext", gfx::Color::from_rgb(0)}, // black
        {"linktext", gfx::Color::from_rgb(0x00'00'ff)}, // blue
        {"visitedtext", gfx::Color::from_rgb(0x80'00'80)}, // purple

        // CSS Level 1.
        {"black", gfx::Color::from_rgb(0)},
        {"silver", gfx::Color::from_rgb(0xc0'c0'c0)},
        {"gray", gfx::Color::from_rgb(0x80'80'80)},
        {"white", gfx::Color::from_rgb(0xff'ff'ff)},
        {"maroon", gfx::Color::from_rgb(0x80'00'00)},
        {"red", gfx::Color::from_rgb(0xff'00'00)},
        {"purple", gfx::Color::from_rgb(0x80'00'80)},
        {"fuchsia", gfx::Color::from_rgb(0xff'00'ff)},
        {"green", gfx::Color::from_rgb(0x00'80'00)},
        {"lime", gfx::Color::from_rgb(0x00'ff'00)},
        {"olive", gfx::Color::from_rgb(0x80'80'00)},
        {"yellow", gfx::Color::from_rgb(0xff'ff'00)},
        {"navy", gfx::Color::from_rgb(0x00'00'80)},
        {"blue", gfx::Color::from_rgb(0x00'00'ff)},
        {"teal", gfx::Color::from_rgb(0x00'80'80)},
        {"aqua", gfx::Color::from_rgb(0x00'ff'ff)},
        // CSS Level 2.
        {"orange", gfx::Color::from_rgb(0xff'a5'00)},
        // CSS Level 3.
        {"aliceblue", gfx::Color::from_rgb(0xf0'f8'ff)},
        {"antiquewhite", gfx::Color::from_rgb(0xfa'eb'd7)},
        {"aquamarine", gfx::Color::from_rgb(0x7f'ff'd4)},
        {"azure", gfx::Color::from_rgb(0xf0'ff'ff)},
        {"beige", gfx::Color::from_rgb(0xf5'f5'dc)},
        {"bisque", gfx::Color::from_rgb(0xff'e4'c4)},
        {"blanchedalmond", gfx::Color::from_rgb(0xff'eb'cd)},
        {"blueviolet", gfx::Color::from_rgb(0x8a'2b'e2)},
        {"brown", gfx::Color::from_rgb(0xa5'2a'2a)},
        {"burlywood", gfx::Color::from_rgb(0xde'b8'87)},
        {"cadetblue", gfx::Color::from_rgb(0x5f'9e'a0)},
        {"chartreuse", gfx::Color::from_rgb(0x7f'ff'00)},
        {"chocolate", gfx::Color::from_rgb(0xd2'69'1e)},
        {"coral", gfx::Color::from_rgb(0xff'7f'50)},
        {"cornflowerblue", gfx::Color::from_rgb(0x64'95'ed)},
        {"cornsilk", gfx::Color::from_rgb(0xff'f8'dc)},
        {"crimson", gfx::Color::from_rgb(0xdc'14'3c)},
        {"cyan", gfx::Color::from_rgb(0x00'ff'ff)},
        {"darkblue", gfx::Color::from_rgb(0x00'00'8b)},
        {"darkcyan", gfx::Color::from_rgb(0x00'8b'8b)},
        {"darkgoldenrod", gfx::Color::from_rgb(0xb8'86'0b)},
        {"darkgray", gfx::Color::from_rgb(0xa9'a9'a9)},
        {"darkgreen", gfx::Color::from_rgb(0x00'64'00)},
        {"darkgrey", gfx::Color::from_rgb(0xa9'a9'a9)},
        {"darkkhaki", gfx::Color::from_rgb(0xbd'b7'6b)},
        {"darkmagenta", gfx::Color::from_rgb(0x8b'00'8b)},
        {"darkolivegreen", gfx::Color::from_rgb(0x55'6b'2f)},
        {"darkorange", gfx::Color::from_rgb(0xff'8c'00)},
        {"darkorchid", gfx::Color::from_rgb(0x99'32'cc)},
        {"darkred", gfx::Color::from_rgb(0x8b'00'00)},
        {"darksalmon", gfx::Color::from_rgb(0xe9'96'7a)},
        {"darkseagreen", gfx::Color::from_rgb(0x8f'bc'8f)},
        {"darkslateblue", gfx::Color::from_rgb(0x48'3d'8b)},
        {"darkslategray", gfx::Color::from_rgb(0x2f'4f'4f)},
        {"darkslategrey", gfx::Color::from_rgb(0x2f'4f'4f)},
        {"darkturquoise", gfx::Color::from_rgb(0x00'ce'd1)},
        {"darkviolet", gfx::Color::from_rgb(0x94'00'd3)},
        {"deeppink", gfx::Color::from_rgb(0xff'14'93)},
        {"deepskyblue", gfx::Color::from_rgb(0x00'bf'ff)},
        {"dimgray", gfx::Color::from_rgb(0x69'69'69)},
        {"dimgrey", gfx::Color::from_rgb(0x69'69'69)},
        {"dodgerblue", gfx::Color::from_rgb(0x1e'90'ff)},
        {"firebrick", gfx::Color::from_rgb(0xb2'22'22)},
        {"floralwhite", gfx::Color::from_rgb(0xff'fa'f0)},
        {"forestgreen", gfx::Color::from_rgb(0x22'8b'22)},
        {"gainsboro", gfx::Color::from_rgb(0xdc'dc'dc)},
        {"ghostwhite", gfx::Color::from_rgb(0xf8'f8'ff)},
        {"gold", gfx::Color::from_rgb(0xff'd7'00)},
        {"goldenrod", gfx::Color::from_rgb(0xda'a5'20)},
        {"greenyellow", gfx::Color::from_rgb(0xad'ff'2f)},
        {"grey", gfx::Color::from_rgb(0x80'80'80)},
        {"honeydew", gfx::Color::from_rgb(0xf0'ff'f0)},
        {"hotpink", gfx::Color::from_rgb(0xff'69'b4)},
        {"indianred", gfx::Color::from_rgb(0xcd'5c'5c)},
        {"indigo", gfx::Color::from_rgb(0x4b'00'82)},
        {"ivory", gfx::Color::from_rgb(0xff'ff'f0)},
        {"khaki", gfx::Color::from_rgb(0xf0'e6'8c)},
        {"lavender", gfx::Color::from_rgb(0xe6'e6'fa)},
        {"lavenderblush", gfx::Color::from_rgb(0xff'f0'f5)},
        {"lawngreen", gfx::Color::from_rgb(0x7c'fc'00)},
        {"lemonchiffon", gfx::Color::from_rgb(0xff'fa'cd)},
        {"lightblue", gfx::Color::from_rgb(0xad'd8'e6)},
        {"lightcoral", gfx::Color::from_rgb(0xf0'80'80)},
        {"lightcyan", gfx::Color::from_rgb(0xe0'ff'ff)},
        {"lightgoldenrodyellow", gfx::Color::from_rgb(0xfa'fa'd2)},
        {"lightgray", gfx::Color::from_rgb(0xd3'd3'd3)},
        {"lightgreen", gfx::Color::from_rgb(0x90'ee'90)},
        {"lightgrey", gfx::Color::from_rgb(0xd3'd3'd3)},
        {"lightpink", gfx::Color::from_rgb(0xff'b6'c1)},
        {"lightsalmon", gfx::Color::from_rgb(0xff'a0'7a)},
        {"lightseagreen", gfx::Color::from_rgb(0x20'b2'aa)},
        {"lightskyblue", gfx::Color::from_rgb(0x87'ce'fa)},
        {"lightslategray", gfx::Color::from_rgb(0x77'88'99)},
        {"lightslategrey", gfx::Color::from_rgb(0x77'88'99)},
        {"lightsteelblue", gfx::Color::from_rgb(0xb0'c4'de)},
        {"lightyellow", gfx::Color::from_rgb(0xff'ff'e0)},
        {"limegreen", gfx::Color::from_rgb(0x32'cd'32)},
        {"linen", gfx::Color::from_rgb(0xfa'f0'e6)},
        {"magenta", gfx::Color::from_rgb(0xff'00'ff)},
        {"mediumaquamarine", gfx::Color::from_rgb(0x66'cd'aa)},
        {"mediumblue", gfx::Color::from_rgb(0x00'00'cd)},
        {"mediumorchid", gfx::Color::from_rgb(0xba'55'd3)},
        {"mediumpurple", gfx::Color::from_rgb(0x93'70'db)},
        {"mediumseagreen", gfx::Color::from_rgb(0x3c'b3'71)},
        {"mediumslateblue", gfx::Color::from_rgb(0x7b'68'ee)},
        {"mediumspringgreen", gfx::Color::from_rgb(0x00'fa'9a)},
        {"mediumturquoise", gfx::Color::from_rgb(0x48'd1'cc)},
        {"mediumvioletred", gfx::Color::from_rgb(0xc7'15'85)},
        {"midnightblue", gfx::Color::from_rgb(0x19'19'70)},
        {"mintcream", gfx::Color::from_rgb(0xf5'ff'fa)},
        {"mistyrose", gfx::Color::from_rgb(0xff'e4'e1)},
        {"moccasin", gfx::Color::from_rgb(0xff'e4'b5)},
        {"navajowhite", gfx::Color::from_rgb(0xff'de'ad)},
        {"oldlace", gfx::Color::from_rgb(0xfd'f5'e6)},
        {"olivedrab", gfx::Color::from_rgb(0x6b'8e'23)},
        {"orangered", gfx::Color::from_rgb(0xff'45'00)},
        {"orchid", gfx::Color::from_rgb(0xda'70'd6)},
        {"palegoldenrod", gfx::Color::from_rgb(0xee'e8'aa)},
        {"palegreen", gfx::Color::from_rgb(0x98'fb'98)},
        {"paleturquoise", gfx::Color::from_rgb(0xaf'ee'ee)},
        {"palevioletred", gfx::Color::from_rgb(0xdb'70'93)},
        {"papayawhip", gfx::Color::from_rgb(0xff'ef'd5)},
        {"peachpuff", gfx::Color::from_rgb(0xff'da'b9)},
        {"peru", gfx::Color::from_rgb(0xcd'85'3f)},
        {"pink", gfx::Color::from_rgb(0xff'c0'cb)},
        {"plum", gfx::Color::from_rgb(0xdd'a0'dd)},
        {"powderblue", gfx::Color::from_rgb(0xb0'e0'e6)},
        {"rosybrown", gfx::Color::from_rgb(0xbc'8f'8f)},
        {"royalblue", gfx::Color::from_rgb(0x41'69'e1)},
        {"saddlebrown", gfx::Color::from_rgb(0x8b'45'13)},
        {"salmon", gfx::Color::from_rgb(0xfa'80'72)},
        {"sandybrown", gfx::Color::from_rgb(0xf4'a4'60)},
        {"seagreen", gfx::Color::from_rgb(0x2e'8b'57)},
        {"seashell", gfx::Color::from_rgb(0xff'f5'ee)},
        {"sienna", gfx::Color::from_rgb(0xa0'52'2d)},
        {"skyblue", gfx::Color::from_rgb(0x87'ce'eb)},
        {"slateblue", gfx::Color::from_rgb(0x6a'5a'cd)},
        {"slategray", gfx::Color::from_rgb(0x70'80'90)},
        {"slategrey", gfx::Color::from_rgb(0x70'80'90)},
        {"snow", gfx::Color::from_rgb(0xff'fa'fa)},
        {"springgreen", gfx::Color::from_rgb(0x00'ff'7f)},
        {"steelblue", gfx::Color::from_rgb(0x46'82'b4)},
        {"tan", gfx::Color::from_rgb(0xd2'b4'8c)},
        {"thistle", gfx::Color::from_rgb(0xd8'bf'd8)},
        {"tomato", gfx::Color::from_rgb(0xff'63'47)},
        {"transparent", {0x00, 0x00, 0x00, 0x00}},
        {"turquoise", gfx::Color::from_rgb(0x40'e0'd0)},
        {"violet", gfx::Color::from_rgb(0xee'82'ee)},
        {"wheat", gfx::Color::from_rgb(0xf5'de'b3)},
        {"whitesmoke", gfx::Color::from_rgb(0xf5'f5'f5)},
        {"yellowgreen", gfx::Color::from_rgb(0x9a'cd'32)},
        // CSS Level 4.
        {"rebeccapurple", gfx::Color::from_rgb(0x66'33'99)},
};

} // namespace

std::optional<Color> Color::from_css_name(std::string_view name) {
    if (!named_colors.contains(name)) {
        return std::nullopt;
    }

    return named_colors.at(name);
}

} // namespace gfx
