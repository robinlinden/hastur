// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_TOKEN_H_
#define JS_TOKEN_H_

#include <cassert>
#include <cstdint>
#include <string>
#include <variant>

namespace js::parse {

struct IntLiteral {
    std::int32_t value{};
    bool operator==(IntLiteral const &) const = default;
};

struct StringLiteral {
    std::string value;
    bool operator==(StringLiteral const &) const = default;
};

struct Identifier {
    std::string name;
    bool operator==(Identifier const &) const = default;
};

struct Comment {
    std::string comment;
    bool operator==(Comment const &) const = default;
};

struct LParen {
    bool operator==(LParen const &) const = default;
};

struct RParen {
    bool operator==(RParen const &) const = default;
};

struct LBrace {
    bool operator==(LBrace const &) const = default;
};

struct RBrace {
    bool operator==(RBrace const &) const = default;
};

struct LBracket {
    bool operator==(LBracket const &) const = default;
};

struct RBracket {
    bool operator==(RBracket const &) const = default;
};

struct Semicolon {
    bool operator==(Semicolon const &) const = default;
};

struct Comma {
    bool operator==(Comma const &) const = default;
};

struct Period {
    bool operator==(Period const &) const = default;
};

struct Equals {
    bool operator==(Equals const &) const = default;
};

struct Plus {
    bool operator==(Plus const &) const = default;
};

struct Asterisk {
    bool operator==(Asterisk const &) const = default;
};

struct Eof {
    bool operator==(Eof const &) const = default;
};

// Reserved words
// https://tc39.es/ecma262/#prod-ReservedWord

struct Await {
    bool operator==(Await const &) const = default;
};

struct Break {
    bool operator==(Break const &) const = default;
};

struct Case {
    bool operator==(Case const &) const = default;
};

struct Catch {
    bool operator==(Catch const &) const = default;
};

struct Class {
    bool operator==(Class const &) const = default;
};

struct Const {
    bool operator==(Const const &) const = default;
};

struct Continue {
    bool operator==(Continue const &) const = default;
};

struct Debugger {
    bool operator==(Debugger const &) const = default;
};

struct Default {
    bool operator==(Default const &) const = default;
};

struct Delete {
    bool operator==(Delete const &) const = default;
};

struct Do {
    bool operator==(Do const &) const = default;
};

struct Else {
    bool operator==(Else const &) const = default;
};

struct Enum {
    bool operator==(Enum const &) const = default;
};

struct Export {
    bool operator==(Export const &) const = default;
};

struct Extends {
    bool operator==(Extends const &) const = default;
};

struct False {
    bool operator==(False const &) const = default;
};

struct Finally {
    bool operator==(Finally const &) const = default;
};

struct For {
    bool operator==(For const &) const = default;
};

struct Function {
    bool operator==(Function const &) const = default;
};

struct If {
    bool operator==(If const &) const = default;
};

struct Import {
    bool operator==(Import const &) const = default;
};

struct In {
    bool operator==(In const &) const = default;
};

struct InstanceOf {
    bool operator==(InstanceOf const &) const = default;
};

struct New {
    bool operator==(New const &) const = default;
};

struct Null {
    bool operator==(Null const &) const = default;
};

struct Return {
    bool operator==(Return const &) const = default;
};

struct Super {
    bool operator==(Super const &) const = default;
};

struct Switch {
    bool operator==(Switch const &) const = default;
};

struct This {
    bool operator==(This const &) const = default;
};

struct Throw {
    bool operator==(Throw const &) const = default;
};

struct True {
    bool operator==(True const &) const = default;
};

struct Try {
    bool operator==(Try const &) const = default;
};

struct TypeOf {
    bool operator==(TypeOf const &) const = default;
};

struct Var {
    bool operator==(Var const &) const = default;
};

struct Void {
    bool operator==(Void const &) const = default;
};

struct While {
    bool operator==(While const &) const = default;
};

struct With {
    bool operator==(With const &) const = default;
};

struct Yield {
    bool operator==(Yield const &) const = default;
};

using Token = std::variant< //
        IntLiteral,
        StringLiteral,
        Identifier,
        Comment,
        LParen,
        RParen,
        LBrace,
        RBrace,
        LBracket,
        RBracket,
        Semicolon,
        Comma,
        Period,
        Equals,
        Plus,
        Asterisk,
        Eof,
        // Reserved words
        Await,
        Break,
        Case,
        Catch,
        Class,
        Const,
        Continue,
        Debugger,
        Default,
        Delete,
        Do,
        Else,
        Enum,
        Export,
        Extends,
        False,
        Finally,
        For,
        Function,
        If,
        Import,
        In,
        InstanceOf,
        New,
        Null,
        Return,
        Super,
        Switch,
        This,
        Throw,
        True,
        Try,
        TypeOf,
        Var,
        Void,
        While,
        With,
        Yield>;

} // namespace js::parse

#endif
