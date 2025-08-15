// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/default.h"

#include "css/default_css.h"
#include "css/parser.h"
#include "css/style_sheet.h"

namespace css {

StyleSheet default_style() {
    return css::parse(kDefaultCss);
}

} // namespace css
