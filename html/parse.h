// SPDX-FileCopyrightText: 2021-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSE_H_
#define HTML_PARSE_H_

#include "html/parser_options.h"

#include "dom/dom.h"

#include <string_view>

namespace html {

dom::Document parse(std::string_view input, ParserOptions const & = {}, Callbacks const & = {});

[[nodiscard]] dom::DocumentFragment parse_fragment(
        dom::Element const &context, std::string_view input, ParserOptions const &, Callbacks const &);

} // namespace html

#endif
