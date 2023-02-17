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

int main() {
    etest::test("decode unsigned", [] {
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\0"s}), std::uint32_t{0});
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\1"}), std::uint32_t{1});
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\x3f"}), std::uint32_t{63});
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\x40"}), std::uint32_t{64});
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\x7f"}), std::uint32_t{0x7f});
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\x80\x01"}), std::uint32_t{0x80});
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\x80\x02"}), std::uint32_t{0x100});
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\x80\x7f"}), std::uint32_t{16256});
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\x81\x01"}), std::uint32_t{0x81});
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\x81\x02"}), std::uint32_t{0x101});
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\x90\x01"}), std::uint32_t{0x90});
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\xff\x01"}), std::uint32_t{0xff});
        expect_eq(Leb128<std::uint64_t>::decode_from(std::stringstream{"\x80\xc1\x80\x80\x10"}), std::uint64_t{4294975616});

        // Missing termination.
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\x80"}), std::nullopt);
        // Too many bytes with no termination.
        expect_eq(Leb128<std::uint32_t>::decode_from(std::stringstream{"\x80\x80\x80\x80\x80\x80"}), std::nullopt);
    });

    return etest::run_all_tests();
}
