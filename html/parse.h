// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSE_H_
#define HTML_PARSE_H_

#include "html/parser_options.h"

#include "dom/dom.h"
#include "html2/parse_error.h"

#include <functional>
#include <string_view>

namespace html {

dom::Document parse(std::string_view input, ParserOptions const &, Callbacks const &);

inline dom::Document parse(
        std::string_view input,
        ParserOptions const &opts = {},
        std::function<void(html2::ParseError)> const &on_error = [](auto) {}) {
    return parse(input, opts, Callbacks{.on_error = on_error});
}

} // namespace html

#endif
