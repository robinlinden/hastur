// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/crc32.h"

#include "etest/etest.h"

#include <span>
#include <string_view>

using namespace std::literals;
using etest::expect_eq;

int main() {
    etest::test("no data", [] { expect_eq(util::crc32(std::span{""sv}), 0u); });

    etest::test("data", [] {
        // crc32 <(echo -n "The quick brown fox jumps over the lazy dog")
        auto data = "The quick brown fox jumps over the lazy dog"sv;
        expect_eq(util::crc32(std::span{data}), 0x414fa339u);
    });

    return etest::run_all_tests();
}
