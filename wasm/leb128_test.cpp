// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/leb128.h"

#include "etest/etest.h"

#include <sstream>
#include <string>

using namespace std::literals;
using etest::expect_eq;
using wasm::Leb128;

namespace {
template<typename T>
void expect_decoded(std::string bytes, T expected, etest::source_location loc = etest::source_location::current()) {
    expect_eq(Leb128<T>::decode_from(std::stringstream{std::move(bytes)}), expected, std::nullopt, std::move(loc));
};

template<typename T>
void expect_decode_failure(std::string bytes, etest::source_location loc = etest::source_location::current()) {
    expect_eq(Leb128<T>::decode_from(std::stringstream{std::move(bytes)}), std::nullopt, std::nullopt, std::move(loc));
};
} // namespace

int main() {
    etest::test("decode unsigned", [] {
        expect_decoded<std::uint32_t>("\0"s, 0);
        expect_decoded<std::uint32_t>("\1", 1);
        expect_decoded<std::uint32_t>("\x3f", 63);
        expect_decoded<std::uint32_t>("\x40", 64);
        expect_decoded<std::uint32_t>("\x7f", 0x7f);
        expect_decoded<std::uint32_t>("\x80\x01", 0x80);
        expect_decoded<std::uint32_t>("\x80\x02", 0x100);
        expect_decoded<std::uint32_t>("\x80\x7f", 16256);
        expect_decoded<std::uint32_t>("\x81\x01", 0x81);
        expect_decoded<std::uint32_t>("\x81\x02", 0x101);
        expect_decoded<std::uint32_t>("\x90\x01", 0x90);
        expect_decoded<std::uint32_t>("\xff\x01", 0xff);
        expect_decoded<std::uint64_t>("\x80\xc1\x80\x80\x10", 4294975616);

        // Missing termination.
        expect_decode_failure<std::uint32_t>("\x80");
        // Too many bytes with no termination.
        expect_decode_failure<std::uint32_t>("\x80\x80\x80\x80\x80\x80");
    });

    return etest::run_all_tests();
}
