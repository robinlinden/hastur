// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/token.h"

#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <variant>

namespace css2 {

namespace {

std::ostream &operator<<(std::ostream &os, HashToken::Type type) {
    switch (type) {
        case HashToken::Type::Unrestricted:
            os << "(unrestricted)";
            break;
        case HashToken::Type::Id:
            os << "(id)";
            break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, std::variant<int, double> const &data) {
    std::visit([&os](auto const &d) { os << d; }, data);
    return os;
}

struct StringifyVisitor {
    std::ostream &ss;

    void operator()(IdentToken const &t) { ss << "IdentToken " << t.data; }
    void operator()(FunctionToken const &t) { ss << "FunctionToken " << t.data; }
    void operator()(AtKeywordToken const &t) { ss << "AtKeywordToken " << t.data; }
    void operator()(HashToken const &t) { ss << "HashToken " << t.data << " " << t.type; }
    void operator()(StringToken const &t) { ss << "StringToken " << t.data; }
    void operator()(BadStringToken const &) { ss << "BadStringToken"; }
    void operator()(UrlToken const &t) { ss << "UrlToken " << t.data; }
    void operator()(BadUrlToken const &) { ss << "BadUrlToken"; }
    void operator()(DelimToken const &t) { ss << "DelimToken " << t.data; }
    void operator()(NumberToken const &t) { ss << "NumberToken " << t.data; }
    void operator()(PercentageToken const &t) { ss << "PercentageToken " << t.data; }
    void operator()(DimensionToken const &t) { ss << "DimensionToken " << t.data << t.unit; }
    void operator()(WhitespaceToken const &) { ss << "WhitespaceToken"; }
    void operator()(CdoToken const &) { ss << "CdoToken"; }
    void operator()(CdcToken const &) { ss << "CdcToken"; }
    void operator()(ColonToken const &) { ss << "ColonToken"; }
    void operator()(SemiColonToken const &) { ss << "SemiColonToken"; }
    void operator()(CommaToken const &) { ss << "CommaToken"; }
    void operator()(OpenSquareToken const &) { ss << "OpenSquareToken"; }
    void operator()(CloseSquareToken const &) { ss << "CloseSquareToken"; }
    void operator()(OpenParenToken const &) { ss << "OpenParenToken"; }
    void operator()(CloseParenToken const &) { ss << "CloseParenToken"; }
    void operator()(OpenCurlyToken const &) { ss << "OpenCurlyToken"; }
    void operator()(CloseCurlyToken const &) { ss << "CloseCurlyToken"; }
};

} // namespace

std::string to_string(Token const &token) {
    std::stringstream ss;
    std::visit(StringifyVisitor{ss}, token);
    return std::move(ss).str();
}

} // namespace css2
