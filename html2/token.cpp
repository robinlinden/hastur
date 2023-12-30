// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/token.h"

#include <fmt/core.h>

#include <string>
#include <variant>

namespace html2 {
namespace {

class TokenStringifier {
public:
    static std::string to_string(Token const &token) { return std::visit(TokenStringifier{}, token); }

    std::string operator()(DoctypeToken const &t) {
        return fmt::format("Doctype {} {} {}",
                t.name.value_or(R"("")"),
                t.public_identifier.value_or(R"("")"),
                t.system_identifier.value_or(R"("")"));
    }
    std::string operator()(StartTagToken const &t) { return fmt::format("StartTag {} {}", t.tag_name, t.self_closing); }
    std::string operator()(EndTagToken const &t) { return fmt::format("EndTag {} {}", t.tag_name, t.self_closing); }
    std::string operator()(CommentToken const &t) { return fmt::format("Comment {}", t.data); }
    std::string operator()(CharacterToken const &t) { return fmt::format("Character {}", t.data); }
    std::string operator()(EndOfFileToken const &) { return "EndOfFile"; }
};

} // namespace

std::string to_string(Token const &token) {
    return TokenStringifier::to_string(token);
}

} // namespace html2
