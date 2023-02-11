// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSE_H_
#define HTML_PARSE_H_

#include "dom/dom.h"

#include <string_view>

namespace html {



// I know this doesn't belong here, it was easier for this draft
struct ExtResource {
    std::string type;
    std::string href;
};

std::tuple<dom::Document, std::vector<ExtResource>> parse(std::string_view input);

} // namespace html

#endif
