// SPDX-FileCopyrightText: 2021-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/parse.h"

#include "html/parser.h"
#include "html/parser_options.h"

#include "dom/dom.h"

#include <string_view>

namespace html {

dom::Document parse(std::string_view input, ParserOptions const &opts, Callbacks const &cbs) {
    return Parser::parse_document(input, opts, cbs);
}

dom::DocumentFragment parse_fragment(
        dom::Element const &context, std::string_view input, ParserOptions const &opts, Callbacks const &cbs) {
    return Parser::parse_fragment(context, input, opts, cbs);
}

} // namespace html
