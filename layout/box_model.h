// SPDX-FileCopyrightText: 2021-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef LAYOUT_BOX_MODEL_H_
#define LAYOUT_BOX_MODEL_H_

#include "geom/geom.h"

namespace layout {

struct BoxModel {
    geom::Rect content{};

    geom::EdgeSize padding{};
    geom::EdgeSize border{};
    geom::EdgeSize margin{};

    [[nodiscard]] bool operator==(BoxModel const &) const = default;

    [[nodiscard]] constexpr geom::Rect padding_box() const { return content.expanded(padding); }
    [[nodiscard]] constexpr geom::Rect border_box() const { return padding_box().expanded(border); }
    [[nodiscard]] constexpr geom::Rect margin_box() const { return border_box().expanded(margin); }

    [[nodiscard]] constexpr bool contains(geom::Position p) const { return border_box().contains(p); }
};

} // namespace layout

#endif
