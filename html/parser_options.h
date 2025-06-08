// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSER_OPTIONS_H_
#define HTML_PARSER_OPTIONS_H_

#include "dom/dom.h"
#include "html2/parse_error.h"

#include <functional>

namespace html {

struct ParserOptions {
    bool scripting{false};
};

struct Callbacks {
    std::function<void(dom::Element const &)> on_element_closed;
    std::function<void(html2::ParseError)> on_error;
};

} // namespace html

#endif
