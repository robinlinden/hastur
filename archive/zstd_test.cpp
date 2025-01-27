// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/zstd.h"

#include "etest/etest2.h"

#include <tl/expected.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {
std::span<std::byte const> as_bytes(std::span<std::uint8_t const> s) {
    return {reinterpret_cast<std::byte const *>(s.data()), s.size()};
}

// "This is a test string\n"
constexpr auto kSuccessTestString = std::to_array<std::uint8_t>({0x28,
        0xb5,
        0x2f,
        0xfd,
        0x04,
        0x00,
        0xb1,
        0x00,
        0x00,
        0x54,
        0x68,
        0x69,
        0x73,
        0x20,
        0x69,
        0x73,
        0x20,
        0x61,
        0x20,
        0x74,
        0x65,
        0x73,
        0x74,
        0x20,
        0x73,
        0x74,
        0x72,
        0x69,
        0x6e,
        0x67,
        0x0a,
        0xd8,
        0x6a,
        0x8c,
        0x62});

} // namespace

int main() {
    etest::Suite s{"zstd"};

    using namespace archive;

    s.add_test("trivial decode", [](etest::IActions &a) {
        auto ret = zstd_decode(as_bytes(kSuccessTestString));

        a.expect(ret.has_value());
        a.expect_eq(std::string(reinterpret_cast<char const *>(ret->data()), ret->size()), "This is a test string\n");
    });

    s.add_test("output too large", [](etest::IActions &a) {
        ZstdDecoder decoder;

        decoder.set_max_output_length(21);
        auto ret = decoder.decode(as_bytes(kSuccessTestString));
        a.expect_eq(ret, tl::unexpected{ZstdError::MaximumOutputLengthExceeded});

        decoder.set_max_output_length(22);
        ret = decoder.decode(as_bytes(kSuccessTestString));
        a.require(ret.has_value());
        a.expect_eq(
                std::string_view{reinterpret_cast<char const *>(ret->data()), ret->size()}, "This is a test string\n");
    });

    s.add_test("empty input", [](etest::IActions &a) {
        auto ret = zstd_decode({});

        a.expect(!ret.has_value());
        a.expect_eq(ret.error(), ZstdError::InputEmpty);
    });

    s.add_test("zero-sized output", [](etest::IActions &a) {
        constexpr auto kCompress = std::to_array<std::uint8_t>(
                {0x28, 0xb5, 0x2f, 0xfd, 0x24, 0x00, 0x01, 0x00, 0x00, 0x99, 0xe9, 0xd8, 0x51});

        auto ret = zstd_decode(as_bytes(kCompress));

        a.expect(ret.has_value());
        a.expect(ret->empty());
    });

    s.add_test("decoding terminates on even chunk size", [](etest::IActions &a) {
        constexpr auto kCompress = std::to_array<std::uint8_t>({0x28,
                0xb5,
                0x2f,
                0xfd,
                0x04,
                0x58,
                0x55,
                0x00,
                0x00,
                0x10,
                0x41,
                0x41,
                0x01,
                0x00,
                0xfb,
                0xff,
                0x39,
                0xc0,
                0x02,
                0xe7,
                0x8e,
                0x9e,
                0xc3});

        auto ret = zstd_decode(as_bytes(kCompress));

        a.expect(ret.has_value());
        a.expect_eq(ret->size(), 131072ul); // ZSTD_DStreamOutSize, the default chunk value

        for (std::byte byte : *ret) {
            a.expect_eq(byte, std::byte{0x41});
        }
    });

    s.add_test("decoding terminates on even chunk size * 2", [](etest::IActions &a) {
        constexpr auto kCompress = std::to_array<std::uint8_t>({0x28,
                0xb5,
                0x2f,
                0xfd,
                0x04,
                0x58,
                0x54,
                0x00,
                0x00,
                0x10,
                0x41,
                0x41,
                0x01,
                0x00,
                0xfb,
                0xff,
                0x39,
                0xc0,
                0x02,
                0x03,
                0x00,
                0x10,
                0x41,
                0x42,
                0x70,
                0xf6,
                0x4a});

        auto ret = zstd_decode(as_bytes(kCompress));

        a.expect(ret.has_value());
        a.expect_eq(ret->size(), 262144ul); // ZSTD_DStreamOutSize * 2

        for (std::byte byte : *ret) {
            a.expect_eq(byte, std::byte{0x41});
        }
    });

    s.add_test("decoding terminates on chunk size + 20", [](etest::IActions &a) {
        constexpr auto kCompress = std::to_array<std::uint8_t>({0x28,
                0xb5,
                0x2f,
                0xfd,
                0x04,
                0x58,
                0x54,
                0x00,
                0x00,
                0x10,
                0x41,
                0x41,
                0x01,
                0x00,
                0xfb,
                0xff,
                0x39,
                0xc0,
                0x02,
                0xa3,
                0x00,
                0x00,
                0x41,
                0x65,
                0xa2,
                0xc2,
                0xad});

        auto ret = zstd_decode(as_bytes(kCompress));

        a.expect(ret.has_value());
        a.expect_eq(ret->size(), 131092ul); // ZSTD_DStreamOutSize + 20

        for (std::byte byte : *ret) {
            a.expect_eq(byte, std::byte{0x41});
        }
    });

    s.add_test("junk input", [](etest::IActions &a) {
        constexpr auto kCompress = std::to_array<std::uint8_t>({0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00});

        auto ret = zstd_decode(as_bytes(kCompress));

        a.expect(!ret.has_value());
        a.expect_eq(ret.error(), ZstdError::ZstdInternalError);
    });

    s.add_test("truncated zstd stream", [](etest::IActions &a) {
        constexpr auto kCompress = std::to_array<std::uint8_t>({0x28,
                0xb5,
                0x2f,
                0xfd,
                0x04,
                0x00,
                0xb1,
                0x00,
                0x00,
                0x54,
                0x68,
                0x69,
                0x73,
                0x20,
                0x69,
                0x73,
                0x20,
                0x61,
                0x20,
                0x74,
                0x65,
                0x73,
                0x74,
                0x20,
                0x73,
                0x74,
                0x72,
                0x69});

        auto ret = zstd_decode(as_bytes(kCompress));

        a.expect(!ret.has_value());
        a.expect_eq(ret.error(), ZstdError::DecodeEarlyTermination);
    });

    s.add_test("all error codes can be printed", [](etest::IActions &a) {
        static constexpr auto kFirstError = ZstdError::DecodeEarlyTermination;
        static constexpr auto kLastError = ZstdError::ZstdInternalError;

        auto error = static_cast<int>(kFirstError);
        a.expect_eq(error, 0);

        while (error <= static_cast<int>(kLastError)) {
            a.expect(to_string(static_cast<ZstdError>(error)) != "Unknown error",
                    std::to_string(error) + " is missing an error message");
            error += 1;
        }

        a.expect_eq(to_string(static_cast<ZstdError>(error + 1)), "Unknown error");
    });

    return s.run();
}
