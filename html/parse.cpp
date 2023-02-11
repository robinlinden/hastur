// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/parse.h"

#include "html/parser.h"

namespace html {

std::tuple<dom::Document, std::vector<ExtResource>> parse(std::string_view input) {
    return Parser::parse_document(input);
}

} // namespace html
