// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/parse.h"

#include "html/parser.h"

namespace html {

dom::Document parse(std::string_view input) {
    return Parser{input}.parse_document();
}

} // namespace html
