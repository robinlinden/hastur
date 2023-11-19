// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef LAYOUT_LAYOUT_H_
#define LAYOUT_LAYOUT_H_

#include "layout/layout_box.h"

#include "style/styled_node.h"
#include "type/naive.h"
#include "type/type.h"

#include <optional>

namespace layout {

std::optional<LayoutBox> create_layout(style::StyledNode const &, int width, type::IType const & = type::NaiveType{});

} // namespace layout

#endif
