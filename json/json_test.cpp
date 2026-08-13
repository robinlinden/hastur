// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "json/json.h"

#include "etest/etest2.h"

#include <algorithm>
#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <variant>

int main() {
    using namespace std::literals;
    using json::Error;
    using json::Value;
    etest::Suite s{};

    // std::to_string isn't constexpr.
    s.add_test("to_string(Error)", [](etest::IActions &a) {
        static constexpr auto kFirstError = Error::InvalidEscape;
        static constexpr auto kLastError = Error::UnpairedSurrogate;

        auto error = static_cast<int>(kFirstError);
        a.expect_eq(error, 0);

        while (error <= static_cast<int>(kLastError)) {
            a.expect(to_string(static_cast<Error>(error)) != "Unknown error",
                    std::to_string(error) + " is missing an error message");
            error += 1;
        }

        a.expect_eq(json::to_string(static_cast<Error>(error + 1)), "Unknown error");
    });

    s.constexpr_test("bad input", [](etest::IActions &a) {
        a.expect_eq(json::parse(""), std::unexpected{Error::UnexpectedEof});
        a.expect_eq(json::parse(","), std::unexpected{Error::UnexpectedCharacter});
    });

    s.constexpr_test("string", [](etest::IActions &a) {
        a.expect_eq(json::parse(R"("hello")"), json::Value{"hello"});
        a.expect_eq(json::parse(R"(     "hello"     )"), json::Value{"hello"});
        a.expect_eq(json::parse("\t\n\r \"hello\"\t\n\r "), json::Value{"hello"});
        a.expect_eq(json::parse(R"("hello",)"), std::unexpected{Error::TrailingGarbage});
        a.expect_eq(json::parse(R"("")"), json::Value{""});
        a.expect_eq(json::parse(R"("hello)"), std::unexpected{Error::UnexpectedEof});
        a.expect_eq(json::parse(R"(")"), std::unexpected{Error::UnexpectedEof});

        // Control characters (where a control character is <= 0x1f) are disallowed.
        a.expect_eq(json::parse("\"\x00\""sv), std::unexpected{Error::UnexpectedControlCharacter});
        a.expect_eq(json::parse("\"\x1f\""), std::unexpected{Error::UnexpectedControlCharacter});
        a.expect_eq(json::parse("\"\x7f\""), json::Value{"\x7f"});
    });

    s.constexpr_test("string, escapes", [](etest::IActions &a) {
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

        a.expect_eq(json::parse(R"("hello\u")"), std::unexpected{Error::InvalidEscape});
        a.expect_eq(json::parse(R"("hello\u123)"), std::unexpected{Error::UnexpectedEof});
        a.expect_eq(json::parse(R"("hello\u004")"), std::unexpected{Error::InvalidEscape});
        a.expect_eq(json::parse(R"("hello\u004G")"), std::unexpected{Error::InvalidEscape});

        a.expect_eq(json::parse(R"("hello\p")"), std::unexpected{Error::UnexpectedCharacter});
        a.expect_eq(json::parse(R"("hello\)"), std::unexpected{Error::UnexpectedEof});

        // Surrogates.
        a.expect_eq(json::parse(R"("\uD852\uDF62")"), json::Value{"𤭢"});
        a.expect_eq(json::parse(R"("\uD852\u0041")"), std::unexpected{Error::UnpairedSurrogate});
        a.expect_eq(json::parse(R"("\uD83D")"), std::unexpected{Error::UnpairedSurrogate});
        a.expect_eq(json::parse(R"("\uDE00")"), std::unexpected{Error::InvalidEscape});
    });

    s.constexpr_test("true", [](etest::IActions &a) {
        a.expect_eq(json::parse("true"), json::Value{true});
        a.expect_eq(json::parse("tru0"), std::unexpected{Error::InvalidKeyword});
        a.expect_eq(json::parse("tr00"), std::unexpected{Error::InvalidKeyword});
        a.expect_eq(json::parse("t000"), std::unexpected{Error::InvalidKeyword});
        a.expect_eq(json::parse("true!"), std::unexpected{Error::TrailingGarbage});
    });

    s.constexpr_test("false", [](etest::IActions &a) {
        a.expect_eq(json::parse("false"), json::Value{false});
        a.expect_eq(json::parse("fals0"), std::unexpected{Error::InvalidKeyword});
        a.expect_eq(json::parse("fal00"), std::unexpected{Error::InvalidKeyword});
        a.expect_eq(json::parse("fa000"), std::unexpected{Error::InvalidKeyword});
        a.expect_eq(json::parse("f0000"), std::unexpected{Error::InvalidKeyword});
        a.expect_eq(json::parse("false!"), std::unexpected{Error::TrailingGarbage});
    });

    s.constexpr_test("null", [](etest::IActions &a) {
        a.expect_eq(json::parse("null"), json::Value{json::Null{}});
        a.expect_eq(json::parse("nul0"), std::unexpected{Error::InvalidKeyword});
        a.expect_eq(json::parse("nu00"), std::unexpected{Error::InvalidKeyword});
        a.expect_eq(json::parse("n000"), std::unexpected{Error::InvalidKeyword});
        a.expect_eq(json::parse("null!"), std::unexpected{Error::TrailingGarbage});
    });

    s.constexpr_test("array", [](etest::IActions &a) {
        a.expect_eq(json::parse("[]"), Value{json::Array{}});
        a.expect_eq(json::parse("[ ]"), Value{json::Array{}});
        a.expect_eq(json::parse(R"(["1"])"), Value{json::Array{{Value{"1"}}}});
        a.expect_eq(json::parse(R"([null, true, "hello", false, []])"),
                Value{json::Array{
                        {Value{json::Null{}}, Value{true}, Value{"hello"}, Value{false}, Value{json::Array{}}},
                }});

        a.expect_eq(json::parse("["), std::unexpected{Error::UnexpectedEof});
        a.expect_eq(json::parse("[blah"), std::unexpected{Error::UnexpectedCharacter});
        a.expect_eq(json::parse("[null a"), std::unexpected{Error::UnexpectedCharacter});
        a.expect_eq(json::parse("[null"), std::unexpected{Error::UnexpectedEof});
        a.expect_eq(json::parse("[null,"), std::unexpected{Error::UnexpectedEof});
    });

    s.constexpr_test("object", [](etest::IActions &a) {
        a.expect_eq(json::parse("{}"), Value{json::Object{}});
        a.expect_eq(json::parse("{ }"), Value{json::Object{}});
        a.expect_eq(json::parse(R"({"key": "value"})"), Value{json::Object{{{"key", Value{"value"}}}}});
        a.expect_eq(json::parse(R"({"key": "value", "key2": "value2"})"),
                Value{json::Object{{{"key", Value{"value"}}, {"key2", Value{"value2"}}}}});
        a.expect_eq(json::parse(R"({"key": true, "key2": "value2", "key3": false})"),
                Value{json::Object{{{"key", Value{true}}, {"key2", Value{"value2"}}, {"key3", Value{false}}}}});

        a.expect_eq(json::parse("{"), std::unexpected{Error::UnexpectedEof});
        a.expect_eq(json::parse("{blah"), std::unexpected{Error::UnexpectedCharacter});
        a.expect_eq(json::parse("{null"), std::unexpected{Error::UnexpectedCharacter});
        a.expect_eq(json::parse(R"({"key")"), std::unexpected{Error::UnexpectedEof});
        a.expect_eq(json::parse(R"({"key"!)"), std::unexpected{Error::UnexpectedCharacter});
        a.expect_eq(json::parse(R"({"key":)"), std::unexpected{Error::UnexpectedEof});
        a.expect_eq(json::parse(R"({"key":asdf)"), std::unexpected{Error::UnexpectedCharacter});
        a.expect_eq(json::parse(R"({"key":true)"), std::unexpected{Error::UnexpectedEof});
        a.expect_eq(json::parse(R"({"key":true,)"), std::unexpected{Error::UnexpectedEof});
        a.expect_eq(json::parse(R"({"key":true a)"), std::unexpected{Error::UnexpectedCharacter});
        a.expect_eq(json::parse(R"({"key":true})"), Value{json::Object{{{"key", Value{true}}}}});
    });

    // MSVC has an issue with constexpr std::expected.
    // etest/etest2.h(154): error C2131: expression did not evaluate to a constant
    // MSVC\14.51.36231\include\expected(661): note: failure was
    // caused by a read of an uninitialized symbol
    // MSVC\14.51.36231\include\expected(661): note: see usage of
    // 'std::expected<json::Value,json::Error>::_Has_value'
    // ...
    // json/json.h(191): note: while evaluating function
    // 'std::expected<json::Value,json::Error> json::Parser::parse_object(int)'
    // json/json.h(327): note: while evaluating function
    // 'std::expected<json::Value,json::Error>::operator bool(void) noexcept const'
#ifndef _MSC_VER
    s.constexpr_test("object, msvc-workaround", [](etest::IActions &a) {
#else
    s.add_test("object, msvc-workaround", [](etest::IActions &a) {
#endif
        a.expect_eq(json::parse(R"({"key": {"key": "value"}})"),
                Value{json::Object{{{"key", Value{json::Object{{{"key", Value{"value"}}}}}}}}});
    });

    s.constexpr_test("object helpers", [](etest::IActions &a) {
        json::Object o{{{"key", Value{"value"}}}};

        a.expect(o.contains("key"));
        a.expect_eq(o.at("key"), Value{"value"});
        a.expect_eq(o.find("key"), std::ranges::find(o.values, "key", &decltype(o.values)::value_type::first));
        a.expect_eq(o.find("blah"), std::ranges::find(o.values, "end", &decltype(o.values)::value_type::first));
    });

    s.constexpr_test("numbers: integrals", [](etest::IActions &a) {
        a.expect_eq(json::parse("0"), Value{0});
        a.expect_eq(json::parse("1"), Value{1});
        a.expect_eq(json::parse("123"), Value{123});
        a.expect_eq(json::parse("-0"), Value{-0});
        a.expect_eq(json::parse("-1"), Value{-1});
        a.expect_eq(json::parse("-123"), Value{-123});

        a.expect_eq(json::parse("123."), std::unexpected{Error::UnexpectedEof});
        a.expect_eq(json::parse("123e"), std::unexpected{Error::UnexpectedEof});
        a.expect_eq(json::parse("123ey"), std::unexpected{Error::UnexpectedCharacter});
        a.expect_eq(json::parse("-a"), std::unexpected{Error::UnexpectedCharacter});
        a.expect_eq(json::parse("1.f"), std::unexpected{Error::UnexpectedCharacter});
    });

    // std::from_chars<double> used in the floating point parser isn't constexpr.
    s.add_test("numbers: floats", [](etest::IActions &a) {
        a.expect_eq(json::parse("123.456"), Value{123.456});
        a.expect_eq(json::parse("-123.456"), Value{-123.456});
        a.expect_eq(json::parse("0.123"), Value{0.123});
        a.expect_eq(json::parse("0.123e4"), Value{0.123e4});
        a.expect_eq(json::parse("0.123e-4"), Value{0.123e-4});
        a.expect_eq(json::parse("0.123e+4"), Value{0.123e+4});

        a.expect_eq(json::parse("0.123e456"), std::unexpected{Error::InvalidNumber}); // out-of-range
        a.expect_eq(json::parse("1234e456"), std::unexpected{Error::InvalidNumber}); // out-of-range
    });

    // The constexpr bits in e.g. MSVC are only happy with recursion up to a
    // point, and we exceed that point when kMaxDepth >= 30.
    s.add_test("deeply nested object", [](etest::IActions &a) {
        static constexpr auto kMaxDepth = 256;
        std::string to_parse;
        for (int i = 0; i < kMaxDepth; ++i) {
            to_parse += R"({"a":)";
        }

        to_parse += R"("b")";

        for (int i = 0; i < kMaxDepth; ++i) {
            to_parse += "}";
        }

        auto json = json::parse(to_parse).value();

        json::Object const *v = std::get_if<json::Object>(&json);
        a.expect(v != nullptr);

        while (v != nullptr && !v->values.empty()) {
            a.expect_eq(v->values[0].first, "a");
            if (!std::holds_alternative<json::Object>(v->values[0].second)) {
                break;
            }

            v = std::get_if<json::Object>(&v->values[0].second);
        }

        a.require_eq(v->values.size(), std::size_t{1});
        a.expect_eq(std::get<std::string>(v->values[0].second), "b");
    });

    s.add_test("deeply nested object, limit hit", [](etest::IActions &a) {
        static constexpr auto kMaxDepth = 300;
        std::string to_parse;
        for (int i = 0; i < kMaxDepth; ++i) {
            to_parse += R"({"a":)";
        }

        to_parse += R"("b")";

        for (int i = 0; i < kMaxDepth; ++i) {
            to_parse += "}";
        }

        a.expect_eq(json::Parser{to_parse}.parse(), std::unexpected{Error::NestingLimitReached});
    });

    s.add_test("deeply nested array", [](etest::IActions &a) {
        static constexpr auto kMaxDepth = 256;
        std::string to_parse;
        for (int i = 0; i < kMaxDepth; ++i) {
            to_parse += "[";
        }

        to_parse += R"("b")";

        for (int i = 0; i < kMaxDepth; ++i) {
            to_parse += "]";
        }

        auto json = json::parse(to_parse).value();

        json::Array const *v = std::get_if<json::Array>(&json);
        a.expect(v != nullptr);

        while (v != nullptr && !v->values.empty()) {
            if (!std::holds_alternative<json::Array>(v->values[0])) {
                break;
            }

            v = std::get_if<json::Array>(&v->values[0]);
        }

        a.require_eq(v->values.size(), std::size_t{1});
        a.expect_eq(std::get<std::string>(v->values[0]), "b");
    });

    s.add_test("deeply nested array, limit hit", [](etest::IActions &a) {
        static constexpr auto kMaxDepth = 300;
        std::string to_parse;
        for (int i = 0; i < kMaxDepth; ++i) {
            to_parse += "[";
        }

        to_parse += R"("b")";

        for (int i = 0; i < kMaxDepth; ++i) {
            to_parse += "]";
        }

        a.expect_eq(json::Parser{to_parse}.parse(), std::unexpected{Error::NestingLimitReached});
    });

    return s.run();
}
