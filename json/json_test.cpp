// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "json/json.h"

#include "etest/etest2.h"

#include <optional>

int main() {
    etest::Suite s{};

    s.add_test("bad input", [](etest::IActions &a) {
        a.expect_eq(json::parse(""), std::nullopt);
        a.expect_eq(json::parse(","), std::nullopt);
    });

    s.add_test("string", [](etest::IActions &a) {
        a.expect_eq(json::parse(R"("hello")"), json::Value{"hello"});
        a.expect_eq(json::parse(R"(     "hello"     )"), json::Value{"hello"});
        a.expect_eq(json::parse("\t\n\r \"hello\"\t\n\r "), json::Value{"hello"});
        a.expect_eq(json::parse(R"("hello",)"), std::nullopt);
        a.expect_eq(json::parse(R"("")"), json::Value{""});
        a.expect_eq(json::parse(R"("hello)"), std::nullopt);
        a.expect_eq(json::parse(R"(")"), std::nullopt);
    });

    s.add_test("string, escapes", [](etest::IActions &a) {
        a.expect_eq(json::parse(R"("hello\n")"), json::Value{"hello\n"});
        a.expect_eq(json::parse(R"("hello\"")"), json::Value{"hello\""});
        a.expect_eq(json::parse(R"("hello\\")"), json::Value{"hello\\"});
        a.expect_eq(json::parse(R"("hello\/")"), json::Value{"hello/"});
        a.expect_eq(json::parse(R"("hello\b")"), json::Value{"hello\b"});
        a.expect_eq(json::parse(R"("hello\f")"), json::Value{"hello\f"});
        a.expect_eq(json::parse(R"("hello\r")"), json::Value{"hello\r"});
        a.expect_eq(json::parse(R"("hello\t")"), json::Value{"hello\t"});
        a.expect_eq(json::parse(R"("hello\u0041")"), json::Value{"helloA"});
        a.expect_eq(json::parse(R"("hello\u004120")"), json::Value{"helloA20"});

        a.expect_eq(json::parse(R"("hello\u")"), std::nullopt);
        a.expect_eq(json::parse(R"("hello\u004")"), std::nullopt);
        a.expect_eq(json::parse(R"("hello\u004G")"), std::nullopt);

        a.expect_eq(json::parse(R"("hello\p")"), std::nullopt);

        // Surrogates.
        a.expect_eq(json::parse(R"("\uD852\uDF62")"), json::Value{"𤭢"});
        a.expect_eq(json::parse(R"("\uD83D")"), std::nullopt);
        a.expect_eq(json::parse(R"("\uDE00")"), std::nullopt);
    });

    s.add_test("true", [](etest::IActions &a) {
        a.expect_eq(json::parse("true"), json::Value{true});
        a.expect_eq(json::parse("tru0"), std::nullopt);
        a.expect_eq(json::parse("tr00"), std::nullopt);
        a.expect_eq(json::parse("t000"), std::nullopt);
        a.expect_eq(json::parse("true!"), std::nullopt);
    });

    s.add_test("false", [](etest::IActions &a) {
        a.expect_eq(json::parse("false"), json::Value{false});
        a.expect_eq(json::parse("fals0"), std::nullopt);
        a.expect_eq(json::parse("fal00"), std::nullopt);
        a.expect_eq(json::parse("fa000"), std::nullopt);
        a.expect_eq(json::parse("f0000"), std::nullopt);
        a.expect_eq(json::parse("false!"), std::nullopt);
    });

    s.add_test("null", [](etest::IActions &a) {
        a.expect_eq(json::parse("null"), json::Value{json::Null{}});
        a.expect_eq(json::parse("nul0"), std::nullopt);
        a.expect_eq(json::parse("nu00"), std::nullopt);
        a.expect_eq(json::parse("n000"), std::nullopt);
        a.expect_eq(json::parse("null!"), std::nullopt);
    });

    return s.run();
}
