// SPDX-FileCopyrightText: 2021-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSE_H_
#define HTML_PARSE_H_

#include "html/parse_error.h"
#include "html/parser_options.h"

#include "dom/dom.h"

#include <functional>
#include <string_view>

namespace html {

dom::Document parse(std::string_view input, ParserOptions const &, Callbacks const &);

inline dom::Document parse(
        std::string_view input,
        ParserOptions const &opts = {},
        std::function<void(ParseError)> const &on_error = [](auto) {}) {
    return parse(input, opts, Callbacks{.on_error = on_error});
}

[[nodiscard]] dom::DocumentFragment parse_fragment(
        dom::Element const &context, std::string_view input, ParserOptions const &, Callbacks const &);

} // namespace html

#endif
