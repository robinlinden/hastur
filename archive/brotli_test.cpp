// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/brotli.h"

#include "etest/etest2.h"

#include <tl/expected.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace {
std::span<std::byte const> as_bytes(std::span<std::uint8_t const> s) {
    return {reinterpret_cast<std::byte const *>(s.data()), s.size()};
}
} // namespace

int main() {
    etest::Suite s{"brotli"};

    using namespace archive;

    s.add_test("empty input",
            [](etest::IActions &a) { a.expect_eq(brotli_decode({}), tl::unexpected{BrotliError::InputEmpty}); });

    s.add_test("trivial decode", [](etest::IActions &a) {
        constexpr auto kCompress = std::to_array<std::uint8_t>(
                {0x1f, 0x0d, 0x00, 0xf8, 0xa5, 0x40, 0xc2, 0xaa, 0x10, 0x49, 0xea, 0x16, 0x85, 0x9c, 0x32, 0x00});

        auto ret = brotli_decode(as_bytes(kCompress));

        a.expect(ret.has_value());
        a.expect_eq(ret->size(), 14ul);
        a.expect_eq(std::string(reinterpret_cast<char const *>(ret->data()), ret->size()), "This is a test");
    });

    s.add_test("input ends at block boundary", [](etest::IActions &a) {
        // python -c "print('A' * 131072, end='')" | brotli
        constexpr auto kCompress = std::to_array<std::uint8_t>(
                {0x5f, 0xff, 0xff, 0x81, 0x5f, 0x22, 0x28, 0x1e, 0x0b, 0x04, 0x72, 0xef, 0x03, 0x00});

        auto ret = brotli_decode(as_bytes(kCompress));

        a.expect(ret.has_value());
        a.expect_eq(ret->size(), 131072ul);

        for (std::byte byte : *ret) {
            a.expect_eq(byte, std::byte{0x41});
        }
    });

    s.add_test("input ends at block boundary * 2", [](etest::IActions &a) {
        // python -c "print('A' * 262144, end='')" | brotli
        constexpr auto kCompress = std::to_array<std::uint8_t>(
                {0x5f, 0xff, 0xff, 0x83, 0x5f, 0x22, 0x28, 0x1e, 0x0b, 0x04, 0x72, 0xef, 0x07, 0x00});

        auto ret = brotli_decode(as_bytes(kCompress));

        a.expect(ret.has_value());
        a.expect_eq(ret->size(), 262144ul);

        for (std::byte byte : *ret) {
            a.expect_eq(byte, std::byte{0x41});
        }
    });

    s.add_test("truncated input", [](etest::IActions &a) {
        constexpr auto kCompress = std::to_array<std::uint8_t>(
                {0x1f, 0x0d, 0x00, 0xf8, 0xa5, 0x40, 0xc2, 0xaa, 0x10, 0x49, 0xea, 0x16, 0x85});

        auto ret = brotli_decode(as_bytes(kCompress));

        a.expect_eq(ret, tl::unexpected{BrotliError::InputCorrupt});
    });

    s.add_test("junk input", [](etest::IActions &a) {
        constexpr auto kCompress =
                std::to_array<std::uint8_t>({0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff});

        auto ret = brotli_decode(as_bytes(kCompress));

        a.expect_eq(ret, tl::unexpected{BrotliError::InputCorrupt});
    });

    s.add_test("zero-sized output", [](etest::IActions &a) {
        constexpr auto kCompress = std::to_array<std::uint8_t>({0x3f});

        auto ret = brotli_decode(as_bytes(kCompress));

        a.expect(ret.has_value());
        a.expect(ret->empty());
    });

    return s.run();
}
