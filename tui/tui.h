// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TUI_TUI_H_
#define TUI_TUI_H_

#include "layout/layout_box.h"

#include <string>

namespace tui {

std::string render(layout::LayoutBox const &root);

} // namespace tui

#endif
