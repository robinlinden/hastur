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

    return s.run();
}
