// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/token.h"

#include "etest/etest.h"

using namespace css2;

int main() {
    etest::test("to_string IdentToken", [] {
        IdentToken t{"foo"};
        etest::expect_eq("IdentToken foo", to_string(t));
    });

    etest::test("to_string FunctionToken", [] {
        FunctionToken t{"foo"};
        etest::expect_eq("FunctionToken foo", to_string(t));
    });

    etest::test("to_string AtKeywordToken", [] {
        AtKeywordToken t{"foo"};
        etest::expect_eq("AtKeywordToken foo", to_string(t));
    });

    etest::test("to_string HashToken id", [] {
        HashToken t{HashToken::Type::Id, "foo"};
        etest::expect_eq("HashToken foo (id)", to_string(t));
    });

    etest::test("to_string HashToken unrestricted", [] {
        HashToken t{HashToken::Type::Unrestricted, "foo"};
        etest::expect_eq("HashToken foo (unrestricted)", to_string(t));
    });

    etest::test("to_string StringToken", [] {
        StringToken t{"foo"};
        etest::expect_eq("StringToken foo", to_string(t));
    });

    etest::test("to_string BadStringToken", [] {
        BadStringToken t{};
        etest::expect_eq("BadStringToken", to_string(t));
    });

    etest::test("to_string DelimToken", [] {
        DelimToken t{','};
        etest::expect_eq("DelimToken ,", to_string(t));
    });

    etest::test("to_string DelimToken", [] {
        DelimToken t{','};
        etest::expect_eq("DelimToken ,", to_string(t));
    });

    etest::test("to_string NumberToken integer", [] {
        NumberToken t{NumericType::Integer, 53};
        etest::expect_eq("NumberToken 53", to_string(t));
    });

    etest::test("to_string NumberToken number", [] {
        NumberToken t{NumericType::Number, 1.33};
        etest::expect_eq("NumberToken 1.33", to_string(t));
    });

    etest::test("to_string PercentageToken integer", [] {
        PercentageToken t{NumericType::Integer, 923};
        etest::expect_eq("PercentageToken 923", to_string(t));
    });

    etest::test("to_string PercentageToken number", [] {
        PercentageToken t{NumericType::Number, 44.123};
        etest::expect_eq("PercentageToken 44.123", to_string(t));
    });

    etest::test("to_string DimensionToken integer", [] {
        DimensionToken t{NumericType::Integer, 1, "px"};
        etest::expect_eq("DimensionToken 1px", to_string(t));
    });

    etest::test("to_string DimensionToken number", [] {
        DimensionToken t{NumericType::Number, 1.5, "em"};
        etest::expect_eq("DimensionToken 1.5em", to_string(t));
    });

    etest::test("to_string WhitespaceToken", [] {
        WhitespaceToken t{};
        etest::expect_eq("WhitespaceToken", to_string(t));
    });

    etest::test("to_string CdoToken", [] {
        CdoToken t{};
        etest::expect_eq("CdoToken", to_string(t));
    });

    etest::test("to_string CdcToken", [] {
        CdcToken t{};
        etest::expect_eq("CdcToken", to_string(t));
    });

    etest::test("to_string ColonToken", [] {
        ColonToken t{};
        etest::expect_eq("ColonToken", to_string(t));
    });

    etest::test("to_string SemiColonToken", [] {
        SemiColonToken t{};
        etest::expect_eq("SemiColonToken", to_string(t));
    });

    etest::test("to_string CommaToken", [] {
        CommaToken t{};
        etest::expect_eq("CommaToken", to_string(t));
    });

    etest::test("to_string OpenSquareToken", [] {
        OpenSquareToken t{};
        etest::expect_eq("OpenSquareToken", to_string(t));
    });

    etest::test("to_string CloseSquareToken", [] {
        CloseSquareToken t{};
        etest::expect_eq("CloseSquareToken", to_string(t));
    });

    etest::test("to_string OpenParenToken", [] {
        OpenParenToken t{};
        etest::expect_eq("OpenParenToken", to_string(t));
    });

    etest::test("to_string CloseParenToken", [] {
        CloseParenToken t{};
        etest::expect_eq("CloseParenToken", to_string(t));
    });

    etest::test("to_string OpenCurlyToken", [] {
        OpenCurlyToken t{};
        etest::expect_eq("OpenCurlyToken", to_string(t));
    });

    etest::test("to_string CloseCurlyToken", [] {
        CloseCurlyToken t{};
        etest::expect_eq("CloseCurlyToken", to_string(t));
    });

    return etest::run_all_tests();
}
