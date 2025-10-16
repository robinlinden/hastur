// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/token.h"

#include <string>
#include <variant>

namespace js::parse {

struct StringifyVisitor {
    std::string operator()(IntLiteral const &t) { return "IntLiteral " + std::to_string(t.value); }
    std::string operator()(StringLiteral const &t) { return "StringLiteral " + t.value; }
    std::string operator()(Identifier const &t) { return "Identifier " + t.name; }
    std::string operator()(Comment const &t) { return "Comment " + t.comment; }
    std::string operator()(LParen const &) { return "LParen"; }
    std::string operator()(RParen const &) { return "RParen"; }
    std::string operator()(LBrace const &) { return "LBrace"; }
    std::string operator()(RBrace const &) { return "RBrace"; }
    std::string operator()(LBracket const &) { return "LBracket"; }
    std::string operator()(RBracket const &) { return "RBracket"; }
    std::string operator()(Semicolon const &) { return "Semicolon"; }
    std::string operator()(Comma const &) { return "Comma"; }
    std::string operator()(Period const &) { return "Period"; }
    std::string operator()(Equals const &) { return "Equals"; }
    std::string operator()(Plus const &) { return "Plus"; }
    std::string operator()(Asterisk const &) { return "Asterisk"; }
    std::string operator()(Eof const &) { return "Eof"; }

    // Reserved words
    std::string operator()(Await const &) { return "Await"; }
    std::string operator()(Break const &) { return "Break"; }
    std::string operator()(Case const &) { return "Case"; }
    std::string operator()(Catch const &) { return "Catch"; }
    std::string operator()(Class const &) { return "Class"; }
    std::string operator()(Const const &) { return "Const"; }
    std::string operator()(Continue const &) { return "Continue"; }
    std::string operator()(Debugger const &) { return "Debugger"; }
    std::string operator()(Default const &) { return "Default"; }
    std::string operator()(Delete const &) { return "Delete"; }
    std::string operator()(Do const &) { return "Do"; }
    std::string operator()(Else const &) { return "Else"; }
    std::string operator()(Enum const &) { return "Enum"; }
    std::string operator()(Export const &) { return "Export"; }
    std::string operator()(Extends const &) { return "Extends"; }
    std::string operator()(False const &) { return "False"; }
    std::string operator()(Finally const &) { return "Finally"; }
    std::string operator()(For const &) { return "For"; }
    std::string operator()(Function const &) { return "Function"; }
    std::string operator()(If const &) { return "If"; }
    std::string operator()(Import const &) { return "Import"; }
    std::string operator()(In const &) { return "In"; }
    std::string operator()(InstanceOf const &) { return "InstanceOf"; }
    std::string operator()(New const &) { return "New"; }
    std::string operator()(Null const &) { return "Null"; }
    std::string operator()(Return const &) { return "Return"; }
    std::string operator()(Super const &) { return "Super"; }
    std::string operator()(Switch const &) { return "Switch"; }
    std::string operator()(This const &) { return "This"; }
    std::string operator()(Throw const &) { return "Throw"; }
    std::string operator()(True const &) { return "True"; }
    std::string operator()(Try const &) { return "Try"; }
    std::string operator()(TypeOf const &) { return "TypeOf"; }
    std::string operator()(Var const &) { return "Var"; }
    std::string operator()(Void const &) { return "Void"; }
    std::string operator()(While const &) { return "While"; }
    std::string operator()(With const &) { return "With"; }
    std::string operator()(Yield const &) { return "Yield"; }
};

std::string to_string(Token const &token) {
    return std::visit(StringifyVisitor{}, token);
}

} // namespace js::parse
