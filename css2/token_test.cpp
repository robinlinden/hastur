// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/token.h"

#include "etest/etest2.h"

using namespace css2;

int main() {
    etest::Suite s{};
    s.add_test("to_string IdentToken", [](etest::IActions &a) {
        IdentToken t{"foo"};
        a.expect_eq("IdentToken foo", to_string(t));
    });

    s.add_test("to_string FunctionToken", [](etest::IActions &a) {
        FunctionToken t{"foo"};
        a.expect_eq("FunctionToken foo", to_string(t));
    });

    s.add_test("to_string AtKeywordToken", [](etest::IActions &a) {
        AtKeywordToken t{"foo"};
        a.expect_eq("AtKeywordToken foo", to_string(t));
    });

    s.add_test("to_string HashToken id", [](etest::IActions &a) {
        HashToken t{HashToken::Type::Id, "foo"};
        a.expect_eq("HashToken foo (id)", to_string(t));
    });

    s.add_test("to_string HashToken unrestricted", [](etest::IActions &a) {
        HashToken t{HashToken::Type::Unrestricted, "foo"};
        a.expect_eq("HashToken foo (unrestricted)", to_string(t));
    });

    s.add_test("to_string StringToken", [](etest::IActions &a) {
        StringToken t{"foo"};
        a.expect_eq("StringToken foo", to_string(t));
    });

    s.add_test("to_string BadStringToken", [](etest::IActions &a) {
        BadStringToken t{};
        a.expect_eq("BadStringToken", to_string(t));
    });

    s.add_test("to_string UrlToken", [](etest::IActions &a) {
        a.expect_eq("UrlToken evilcorp.ltd", to_string(UrlToken{"evilcorp.ltd"})); //
    });

    s.add_test("to_string BadUrlToken", [](etest::IActions &a) {
        a.expect_eq("BadUrlToken", to_string(BadUrlToken{})); //
    });

    s.add_test("to_string DelimToken", [](etest::IActions &a) {
        DelimToken t{','};
        a.expect_eq("DelimToken ,", to_string(t));
    });

    s.add_test("to_string NumberToken integer", [](etest::IActions &a) {
        NumberToken t{NumericType::Integer, 53};
        a.expect_eq("NumberToken 53", to_string(t));
    });

    s.add_test("to_string NumberToken number", [](etest::IActions &a) {
        NumberToken t{NumericType::Number, 1.33};
        a.expect_eq("NumberToken 1.33", to_string(t));
    });

    s.add_test("to_string PercentageToken integer", [](etest::IActions &a) {
        PercentageToken t{NumericType::Integer, 923};
        a.expect_eq("PercentageToken 923", to_string(t));
    });

    s.add_test("to_string PercentageToken number", [](etest::IActions &a) {
        PercentageToken t{NumericType::Number, 44.123};
        a.expect_eq("PercentageToken 44.123", to_string(t));
    });

    s.add_test("to_string DimensionToken integer", [](etest::IActions &a) {
        DimensionToken t{NumericType::Integer, 1, "px"};
        a.expect_eq("DimensionToken 1px", to_string(t));
    });

    s.add_test("to_string DimensionToken number", [](etest::IActions &a) {
        DimensionToken t{NumericType::Number, 1.5, "em"};
        a.expect_eq("DimensionToken 1.5em", to_string(t));
    });

    s.add_test("to_string WhitespaceToken", [](etest::IActions &a) {
        WhitespaceToken t{};
        a.expect_eq("WhitespaceToken", to_string(t));
    });

    s.add_test("to_string CdoToken", [](etest::IActions &a) {
        CdoToken t{};
        a.expect_eq("CdoToken", to_string(t));
    });

    s.add_test("to_string CdcToken", [](etest::IActions &a) {
        CdcToken t{};
        a.expect_eq("CdcToken", to_string(t));
    });

    s.add_test("to_string ColonToken", [](etest::IActions &a) {
        ColonToken t{};
        a.expect_eq("ColonToken", to_string(t));
    });

    s.add_test("to_string SemiColonToken", [](etest::IActions &a) {
        SemiColonToken t{};
        a.expect_eq("SemiColonToken", to_string(t));
    });

    s.add_test("to_string CommaToken", [](etest::IActions &a) {
        CommaToken t{};
        a.expect_eq("CommaToken", to_string(t));
    });

    s.add_test("to_string OpenSquareToken", [](etest::IActions &a) {
        OpenSquareToken t{};
        a.expect_eq("OpenSquareToken", to_string(t));
    });

    s.add_test("to_string CloseSquareToken", [](etest::IActions &a) {
        CloseSquareToken t{};
        a.expect_eq("CloseSquareToken", to_string(t));
    });

    s.add_test("to_string OpenParenToken", [](etest::IActions &a) {
        OpenParenToken t{};
        a.expect_eq("OpenParenToken", to_string(t));
    });

    s.add_test("to_string CloseParenToken", [](etest::IActions &a) {
        CloseParenToken t{};
        a.expect_eq("CloseParenToken", to_string(t));
    });

    s.add_test("to_string OpenCurlyToken", [](etest::IActions &a) {
        OpenCurlyToken t{};
        a.expect_eq("OpenCurlyToken", to_string(t));
    });

    s.add_test("to_string CloseCurlyToken", [](etest::IActions &a) {
        CloseCurlyToken t{};
        a.expect_eq("CloseCurlyToken", to_string(t));
    });

    return s.run();
}
