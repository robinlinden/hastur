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
        expect_decoded<std::uint32_t>("\x80\x7f", 16256);

        // Missing termination.
        expect_decode_failure<std::uint32_t>("\x80");
        // Too many bytes with no termination.
        expect_decode_failure<std::uint32_t>("\x80\x80\x80\x80\x80\x80");

        // https://github.com/llvm/llvm-project/blob/34aff47521c3e0cbac58b0d5793197f76a304295/llvm/unittests/Support/LEB128Test.cpp#L119-L142
        expect_decoded<std::uint32_t>("\0"s, 0);
        expect_decoded<std::uint32_t>("\1", 1);
        expect_decoded<std::uint32_t>("\x3f", 63);
        expect_decoded<std::uint32_t>("\x40", 64);
        expect_decoded<std::uint32_t>("\x7f", 0x7f);
        expect_decoded<std::uint32_t>("\x80\x01", 0x80);
        expect_decoded<std::uint32_t>("\x81\x01", 0x81);
        expect_decoded<std::uint32_t>("\x90\x01", 0x90);
        expect_decoded<std::uint32_t>("\xff\x01", 0xff);
        expect_decoded<std::uint32_t>("\x80\x02", 0x100);
        expect_decoded<std::uint32_t>("\x81\x02", 0x101);
        expect_decoded<std::uint64_t>("\x80\xc1\x80\x80\x10", 4294975616);

        expect_decoded<std::uint64_t>("\x80\x00"s, 0u);
        expect_decoded<std::uint64_t>("\x80\x80\x00"s, 0u);
        expect_decoded<std::uint64_t>("\xff\x00"s, 0x7fu);
        expect_decoded<std::uint64_t>("\xff\x80\x00"s, 0x7fu);
        expect_decoded<std::uint64_t>("\x80\x81\x00"s, 0x80u);
        expect_decoded<std::uint64_t>("\x80\x81\x80\x00"s, 0x80u);
        expect_decoded<std::uint64_t>("\x80\x81\x80\x80\x80\x80\x80\x80\x80\x00"s, 0x80u);
        expect_decoded<std::uint64_t>("\x80\x80\x80\x80\x80\x80\x80\x80\x80\x01", 0x80000000'00000000ul);

        // https://github.com/llvm/llvm-project/blob/34aff47521c3e0cbac58b0d5793197f76a304295/llvm/unittests/Support/LEB128Test.cpp#L160-L166
        // Buffer overflow.
        expect_decode_failure<std::uint64_t>("");
        expect_decode_failure<std::uint64_t>("\x80");

        // Does not fit in 64 bits.
        expect_decode_failure<std::uint64_t>("\x80\x80\x80\x80\x80\x80\x80\x80\x80\x02");
        expect_decode_failure<std::uint64_t>("\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x02");
    });

    etest::test("trailing zeros", [] {
        // From https://webassembly.github.io/spec/core/bikeshed/#binary-int

        // The side conditions N>7 in the productions for non-terminal bytes of
        // the u and s encodings restrict the encoding's length. However,
        // "trailing zeros" are still allowed within these bounds

        // For example, 0x03 and 0x83 0x00 are both well-formed encodings for
        // the value 3 as a u8.
        expect_decoded<std::uint8_t>("\x03", 3);
        expect_decoded<std::uint8_t>("\x83\x00"s, 3);

        // Similarly, either of 0x7e and 0xFE 0x7F and 0xFE 0xFF 0x7F are
        // well-formed encodings of the value -2 as a s16.
        // expect_decoded<std::int16_t>("\x7e", -2);
        // expect_decoded<std::int16_t>("\xfe\x7f", -2);
        // expect_decoded<std::int16_t>("\xfe\xff\x7f", -2);
    });

    etest::test("unused bits in terminal byte", [] {
        // From https://webassembly.github.io/spec/core/bikeshed/#binary-int

        // The side conditions on the value n of terminal bytes further enforce
        // that any unused bits in these bytes must be 0 for positive values and
        // 1 for negative ones.

        // For example, 0x83 0x10 is malformed as a u8 encoding.
        expect_decode_failure<std::uint8_t>("\x83\x10");

        // Similarly, both 0x83 0x3E and 0xFF 0x7B are malformed as s8 encodings
        // expect_decode_failure<std::int8_t>("\x83\x3e");
        // expect_decode_failure<std::int8_t>("\xff\x7b");
    });

    return etest::run_all_tests();
}
