// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "json/json.h"

#include "etest/etest2.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

int main() {
    using namespace std::literals;
    using json::Value;
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

        // Control characters (where a control character is <= 0x1f) are disallowed.
        a.expect_eq(json::parse("\"\x00\""sv), std::nullopt);
        a.expect_eq(json::parse("\"\x1f\""), std::nullopt);
        a.expect_eq(json::parse("\"\x7f\""), json::Value{"\x7f"});
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

    s.add_test("array", [](etest::IActions &a) {
        a.expect_eq(json::parse("[]"), Value{json::Array{}});
        a.expect_eq(json::parse("[ ]"), Value{json::Array{}});
        a.expect_eq(json::parse(R"(["1"])"), Value{json::Array{{Value{"1"}}}});
        a.expect_eq(json::parse(R"([null, true, "hello", false, []])"),
                Value{json::Array{
                        {Value{json::Null{}}, Value{true}, Value{"hello"}, Value{false}, Value{json::Array{}}},
                }});

        a.expect_eq(json::parse("["), std::nullopt);
        a.expect_eq(json::parse("[blah"), std::nullopt);
        a.expect_eq(json::parse("[null"), std::nullopt);
        a.expect_eq(json::parse("[null,"), std::nullopt);
    });

    s.add_test("object", [](etest::IActions &a) {
        a.expect_eq(json::parse("{}"), Value{json::Object{}});
        a.expect_eq(json::parse("{ }"), Value{json::Object{}});
        a.expect_eq(json::parse(R"({"key": "value"})"), Value{json::Object{{{"key", Value{"value"}}}}});
        a.expect_eq(json::parse(R"({"key": "value", "key2": "value2"})"),
                Value{json::Object{{{"key", Value{"value"}}, {"key2", Value{"value2"}}}}});
        a.expect_eq(json::parse(R"({"key": true, "key2": "value2", "key3": false})"),
                Value{json::Object{{{"key", Value{true}}, {"key2", Value{"value2"}}, {"key3", Value{false}}}}});

        a.expect_eq(json::parse(R"({"key": {"key": "value"}})"),
                Value{json::Object{{{"key", Value{json::Object{{{"key", Value{"value"}}}}}}}}});

        a.expect_eq(json::parse("{"), std::nullopt);
        a.expect_eq(json::parse("{blah"), std::nullopt);
        a.expect_eq(json::parse("{null"), std::nullopt);
        a.expect_eq(json::parse(R"({"key")"), std::nullopt);
        a.expect_eq(json::parse(R"({"key":)"), std::nullopt);
        a.expect_eq(json::parse(R"({"key":asdf)"), std::nullopt);
        a.expect_eq(json::parse(R"({"key":true)"), std::nullopt);
        a.expect_eq(json::parse(R"({"key":true,)"), std::nullopt);
        a.expect_eq(json::parse(R"({"key":true})"), Value{json::Object{{{"key", Value{true}}}}});
    });

    s.add_test("object helpers", [](etest::IActions &a) {
        json::Object o{{{"key", Value{"value"}}}};

        a.expect(o.contains("key"));
        a.expect_eq(o.at("key"), Value{"value"});
        a.expect_eq(o.find("key"), std::ranges::find(o.values, "key", &decltype(o.values)::value_type::first));
        a.expect_eq(o.find("blah"), std::ranges::find(o.values, "end", &decltype(o.values)::value_type::first));
    });

    s.add_test("numbers", [](etest::IActions &a) {
        a.expect_eq(json::parse("0"), Value{0});
        a.expect_eq(json::parse("1"), Value{1});
        a.expect_eq(json::parse("123"), Value{123});
        a.expect_eq(json::parse("123.456"), Value{123.456});
        a.expect_eq(json::parse("-0"), Value{-0});
        a.expect_eq(json::parse("-1"), Value{-1});
        a.expect_eq(json::parse("-123"), Value{-123});
        a.expect_eq(json::parse("-123.456"), Value{-123.456});
        a.expect_eq(json::parse("0.123"), Value{0.123});
        a.expect_eq(json::parse("0.123e4"), Value{0.123e4});
        a.expect_eq(json::parse("0.123e-4"), Value{0.123e-4});
        a.expect_eq(json::parse("0.123e+4"), Value{0.123e+4});

        a.expect_eq(json::parse("0.123e456"), std::nullopt); // out-of-range
        a.expect_eq(json::parse("123."), std::nullopt);
    });

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

        a.expect_eq(json::Parser{to_parse}.parse(), std::nullopt);
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

        a.expect_eq(json::Parser{to_parse}.parse(), std::nullopt);
    });

    return s.run();
}
