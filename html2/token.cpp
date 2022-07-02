// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/token.h"

#include "util/overloaded.h"

#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <variant>

namespace html2 {

std::string to_string(Token const &token) {
    auto try_print = []<typename T>(std::ostream &os, std::optional<T> maybe_value) {
        if (maybe_value) {
            os << ' ' << *maybe_value;
        } else {
            os << " \"\"";
        }
    };

    std::stringstream ss;
    std::visit(util::Overloaded{
                       [&](DoctypeToken const &t) {
                           ss << "Doctype";
                           try_print(ss, t.name);
                           try_print(ss, t.public_identifier);
                           try_print(ss, t.system_identifier);
                       },
                       [&ss](StartTagToken const &t) { ss << "StartTag " << t.tag_name << ' ' << t.self_closing; },
                       [&ss](EndTagToken const &t) { ss << "EndTag " << t.tag_name << ' ' << t.self_closing; },
                       [&ss](CommentToken const &t) { ss << "Comment " << t.data; },
                       [&ss](CharacterToken const &t) { ss << "Character " << t.data; },
                       [&ss](EndOfFileToken const &) { ss << "EndOfFile"; },
               },
            token);
    return ss.str();
}

} // namespace html2
