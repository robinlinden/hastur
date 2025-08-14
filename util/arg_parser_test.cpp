// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/arg_parser.h"

#include "etest/etest2.h"

#include <array>
#include <string>

namespace {

void no_args_tests(etest::Suite &s) {
    s.add_test("no args", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        auto res = util::ArgParser{}.parse(kArgC, kArgV.data());
        a.expect(res.has_value());
    });

    s.add_test("no args, argv[0] is nullptr", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({nullptr});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        auto res = util::ArgParser{}.parse(kArgC, kArgV.data());
        a.expect(res.has_value());
    });
}

void positional_tests(etest::Suite &s) {
    s.add_test("positional", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello", "this is great"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        std::string out;
        auto res = util::ArgParser{}.positional(out).parse(kArgC, kArgV.data());

        a.expect(res.has_value());
        a.expect_eq(out, "this is great");
    });

    s.add_test("positional, no args", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        std::string out = "no args";
        auto res = util::ArgParser{}.positional(out).parse(kArgC, kArgV.data());

        a.expect(res.has_value());
        a.expect_eq(out, "no args");
    });

    s.add_test("positional, several args", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello", "this", "is great"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        std::string first;
        std::string second;
        auto res = util::ArgParser{}.positional(first).positional(second).parse(kArgC, kArgV.data());

        a.expect(res.has_value());
        a.expect_eq(first, "this");
        a.expect_eq(second, "is great");
    });

    s.add_test("positional, unhandled", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello", "unhandled!"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        auto res = util::ArgParser{}.parse(kArgC, kArgV.data());

        a.require(!res.has_value());
        a.expect_eq(res.error().code, util::ArgParseError::Code::UnhandledArgument);
        a.expect_eq(res.error().message, "Unhandled argument: unhandled!");
    });
}

void bool_tests(etest::Suite &s) {
    s.add_test("bool, no args", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        bool was_passed = false;
        auto res = util::ArgParser{}.argument("--flag", was_passed).parse(kArgC, kArgV.data());

        a.expect(res.has_value());
        a.expect(!was_passed);
    });

    s.add_test("bool, with args", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello", "--flag"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        bool was_passed = false;
        auto res = util::ArgParser{}.argument("--flag", was_passed).parse(kArgC, kArgV.data());

        a.expect(res.has_value());
        a.expect(was_passed);
    });

    s.add_test("bool, with args, extra unhandled args", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello", "--flag", "extra"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        bool was_passed = false;
        auto res = util::ArgParser{}.argument("--flag", was_passed).parse(kArgC, kArgV.data());

        a.require(!res.has_value());
        a.expect_eq(res.error().code, util::ArgParseError::Code::UnhandledArgument);
        a.expect_eq(res.error().message, "Unhandled argument: extra");
    });

    s.add_test("bool, with args, extra unhandled (before)", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello", "extra", "--flag"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        bool was_passed = false;
        auto res = util::ArgParser{}.argument("--flag", was_passed).parse(kArgC, kArgV.data());

        a.require(!res.has_value());
        a.expect_eq(res.error().code, util::ArgParseError::Code::UnhandledArgument);
        a.expect_eq(res.error().message, "Unhandled argument: extra");
    });
}

void int_tests(etest::Suite &s) {
    s.add_test("int, no args", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        int value = 0;
        auto res = util::ArgParser{}.argument("--value", value).parse(kArgC, kArgV.data());

        a.expect(res.has_value());
        a.expect_eq(value, 0);
    });

    s.add_test("int, with args", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello", "--value", "42"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        int value = 0;
        auto res = util::ArgParser{}.argument("--value", value).parse(kArgC, kArgV.data());

        a.expect(res.has_value());
        a.expect_eq(value, 42);
    });

    s.add_test("int, with invalid args", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello", "--value", "notanumber"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        int value = 0;
        auto res = util::ArgParser{}.argument("--value", value).parse(kArgC, kArgV.data());

        a.require(!res.has_value());
        a.expect_eq(res.error().code, util::ArgParseError::Code::InvalidArgument);
        a.expect_eq(res.error().message, "Invalid argument for --value: notanumber");
    });

    s.add_test("int, with invalid arg starting with digits", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello", "--value", "42notanumber"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        int value = 0;
        auto res = util::ArgParser{}.argument("--value", value).parse(kArgC, kArgV.data());

        a.require(!res.has_value());
        a.expect_eq(res.error().code, util::ArgParseError::Code::InvalidArgument);
        a.expect_eq(res.error().message, "Invalid argument for --value: 42notanumber");
    });

    s.add_test("int, with args, extra unhandled args", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello", "--value", "42", "extra"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        int value = 0;
        auto res = util::ArgParser{}.argument("--value", value).parse(kArgC, kArgV.data());

        a.require(!res.has_value());
        a.expect_eq(res.error().code, util::ArgParseError::Code::UnhandledArgument);
        a.expect_eq(res.error().message, "Unhandled argument: extra");
    });

    s.add_test("int, missing argument", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello", "--value"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        int value = 0;
        auto res = util::ArgParser{}.argument("--value", value).parse(kArgC, kArgV.data());

        a.require(!res.has_value());
        a.expect_eq(res.error().code, util::ArgParseError::Code::MissingArgument);
        a.expect_eq(res.error().message, "Missing argument for --value");
    });
}

void string_tests(etest::Suite &s) {
    s.add_test("string, no args", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        std::string value;
        auto res = util::ArgParser{}.argument("--value", value).parse(kArgC, kArgV.data());

        a.expect(res.has_value());
        a.expect(value.empty());
    });

    s.add_test("string, with args", [](etest::IActions &a) {
        constexpr auto kArgV = std::to_array<char const *>({"hello", "--value", "42"});
        constexpr int kArgC = static_cast<int>(kArgV.size());

        std::string value;
        auto res = util::ArgParser{}.argument("--value", value).parse(kArgC, kArgV.data());

        a.expect(res.has_value());
        a.expect_eq(value, "42");
    });
}

} // namespace

int main() {
    etest::Suite s;

    no_args_tests(s);
    positional_tests(s);
    bool_tests(s);
    int_tests(s);
    string_tests(s);

    return s.run();
}
