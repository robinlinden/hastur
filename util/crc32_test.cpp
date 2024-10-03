// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/crc32.h"

#include "etest/etest2.h"

#include <span>
#include <string_view>

using namespace std::literals;

int main() {
    etest::Suite s;
    s.add_test("no data", [](etest::IActions &a) {
        a.expect_eq(util::crc32(std::span{""sv}), 0u); //
    });

    s.add_test("data", [](etest::IActions &a) {
        // crc32 <(echo -n "The quick brown fox jumps over the lazy dog")
        auto data = "The quick brown fox jumps over the lazy dog"sv;
        a.expect_eq(util::crc32(std::span{data}), 0x414fa339u);
    });

    return s.run();
}
