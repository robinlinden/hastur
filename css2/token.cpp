// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/token.h"

#include "util/overloaded.h"

#include <sstream>
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

} // namespace

std::string to_string(Token const &token) {
    std::stringstream ss;
    std::visit(util::Overloaded{
                       [&ss](IdentToken const &t) { ss << "IdentToken " << t.data; },
                       [&ss](FunctionToken const &t) { ss << "FunctionToken " << t.data; },
                       [&ss](AtKeywordToken const &t) { ss << "AtKeywordToken " << t.data; },
                       [&ss](HashToken const &t) { ss << "HashToken " << t.data << " " << t.type; },
                       [&ss](StringToken const &t) { ss << "StringToken " << t.data; },
                       [&ss](BadStringToken const &) { ss << "BadStringToken"; },
                       [&ss](UrlToken const &t) { ss << "UrlToken " << t.data; },
                       [&ss](BadUrlToken const &) { ss << "BadUrlToken"; },
                       [&ss](DelimToken const &t) { ss << "DelimToken " << t.data; },
                       [&ss](NumberToken const &t) { ss << "NumberToken " << t.data; },
                       [&ss](PercentageToken const &t) { ss << "PercentageToken " << t.data; },
                       [&ss](DimensionToken const &t) { ss << "DimensionToken " << t.data << t.unit; },
                       [&ss](WhitespaceToken const &) { ss << "WhitespaceToken"; },
                       [&ss](CdoToken const &) { ss << "CdoToken"; },
                       [&ss](CdcToken const &) { ss << "CdcToken"; },
                       [&ss](ColonToken const &) { ss << "ColonToken"; },
                       [&ss](SemiColonToken const &) { ss << "SemiColonToken"; },
                       [&ss](CommaToken const &) { ss << "CommaToken"; },
                       [&ss](OpenSquareToken const &) { ss << "OpenSquareToken"; },
                       [&ss](CloseSquareToken const &) { ss << "CloseSquareToken"; },
                       [&ss](OpenParenToken const &) { ss << "OpenParenToken"; },
                       [&ss](CloseParenToken const &) { ss << "CloseParenToken"; },
                       [&ss](OpenCurlyToken const &) { ss << "OpenCurlyToken"; },
                       [&ss](CloseCurlyToken const &) { ss << "CloseCurlyToken"; },
               },
            token);
    return std::move(ss).str();
}

} // namespace css2
