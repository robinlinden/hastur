// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef LAYOUT_LAYOUT_H_
#define LAYOUT_LAYOUT_H_

#include "layout/layout_box.h"

#include "style/styled_node.h"

#include <optional>

namespace layout {

// Being able to toggle this is a debug feature that will go away once both it
// and text wrapping are more ready.
enum class WhitespaceMode {
    Preserve,
    Collapse,
};

std::optional<LayoutBox> create_layout(style::StyledNode const &, int width, WhitespaceMode = WhitespaceMode::Collapse);

} // namespace layout

#endif
