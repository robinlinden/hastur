// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/default.h"

#include "css/parser.h"

#include <string_view>

namespace css {
namespace {
#include "css/default_css.h"
} // namespace

std::vector<css::Rule> default_style() {
    return css::parse(std::string_view{reinterpret_cast<char const *>(css_default_css), css_default_css_len});
}

} // namespace css
