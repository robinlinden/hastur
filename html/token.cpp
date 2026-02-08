// SPDX-FileCopyrightText: 2021-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/token.h"

#include "util/string.h"

#include <format>
#include <string>
#include <variant>

namespace html {
namespace {

class TokenStringifier {
public:
    static std::string to_string(Token const &token) { return std::visit(TokenStringifier{}, token); }

    std::string operator()(DoctypeToken const &t) {
        return std::format("Doctype {} {} {}",
                t.name.value_or(R"("")"),
                t.public_identifier.value_or(R"("")"),
                t.system_identifier.value_or(R"("")"));
    }
    std::string operator()(StartTagToken const &t) { return std::format("StartTag {} {}", t.tag_name, t.self_closing); }
    std::string operator()(EndTagToken const &t) { return std::format("EndTag {}", t.tag_name); }
    std::string operator()(CommentToken const &t) { return std::format("Comment {}", t.data); }
    std::string operator()(CharacterToken const &t) {
        if (util::is_printable(t.data)) {
            return std::format("Character '{}'", t.data);
        }

        return std::format("Character 0x{:02X}", t.data);
    }

    std::string operator()(EndOfFileToken const &) { return "EndOfFile"; }
};

} // namespace

std::string to_string(Token const &token) {
    return TokenStringifier::to_string(token);
}

} // namespace html
