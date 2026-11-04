// SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

#include <expected>
#include <format>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace {

struct CustomPrintable {
    int value{};
};

[[maybe_unused]] std::string to_string(CustomPrintable const &v) {
    return std::format("CustomPrintable{{{}}}", v.value);
}

std::string printed(auto const &lhs, std::string_view op, auto const &rhs) {
    std::ostringstream os;
    etest::print_to(os, op, lhs, rhs);
    return os.str();
}

} // namespace

int main() {
    etest::Suite s{};

    s.add_test("boring", [](etest::IActions &a) {
        a.expect_eq(printed(1, "!=", 2), "1 !=\n2"); //
    });

    s.add_test("optional", [](etest::IActions &a) {
        std::optional<CustomPrintable> const present{CustomPrintable{42}};
        std::optional<CustomPrintable> const empty;

        a.expect_eq(printed(present, "!=", empty), "CustomPrintable{42} !=\nnullopt");
        a.expect_eq(printed(present, "!=", std::nullopt), "CustomPrintable{42} !=\nnullopt");
    });

    s.add_test("expected", [](etest::IActions &a) {
        std::expected<CustomPrintable, CustomPrintable> const value{CustomPrintable{42}};
        std::expected<CustomPrintable, CustomPrintable> const error{std::unexpect, CustomPrintable{24}};

        a.expect_eq(printed(value, "!=", error), "CustomPrintable{42} !=\nunexpected{CustomPrintable{24}}");
    });

    s.add_test("expected void", [](etest::IActions &a) {
        std::expected<void, CustomPrintable> const value;
        std::expected<void, CustomPrintable> const error{std::unexpect, CustomPrintable{24}};

        a.expect_eq(printed(value, "!=", error), "void !=\nunexpected{CustomPrintable{24}}");
    });

    return s.run();
}
