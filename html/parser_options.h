// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSER_OPTIONS_H_
#define HTML_PARSER_OPTIONS_H_

#include "html/parse_error.h"

#include "dom/dom.h"

#include <functional>

namespace html {

struct ParserOptions {
    bool scripting{false};
    bool include_comments{false};
};

struct Callbacks {
    std::function<void(dom::Element const &)> on_element_closed;
    std::function<void(ParseError)> on_error;
};

} // namespace html

#endif
