// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

#include <string>

namespace {

struct Unprintable {
    int a{};
    bool operator==(Unprintable const &) const = default;
};

struct ToStringable {
    int a{};
    bool operator==(ToStringable const &) const = default;
};

std::string to_string(ToStringable const &t) {
    return "ToStringable{" + std::to_string(t.a) + "}";
}

} // namespace

int main() {
    etest::Suite s{};
    s.add_test("basic failure", [](etest::IActions &a) { a.expect_eq(1, 2); });
    bool failed = s.run() == 1;

    etest::Suite s2{};
    s2.add_test("unprintable failure", [](etest::IActions &a) { a.expect_eq(Unprintable{1}, Unprintable{2}); });
    failed &= s2.run() == 1;

    etest::Suite s3{};
    s3.add_test("printable failure", [](etest::IActions &a) { a.expect_eq(ToStringable{1}, ToStringable{2}); });
    failed &= s3.run() == 1;

    return failed ? 1 : 0;
}
