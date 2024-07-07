// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/zlib.h"

#include "etest/etest2.h"

#include <algorithm>
#include <cstddef>
#include <span>
#include <string_view>

using namespace archive;
using namespace std::literals;

namespace {

std::span<std::byte const> as_bytes(std::string_view s) {
    return {reinterpret_cast<std::byte const *>(s.data()), s.size()};
}

constexpr auto kExpected = "p { font-size: 123em; }\n"sv;

// p { font-size: 123em; }, gzipped.
constexpr auto kGzippedCss =
        "\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\x03\x2b\x50\xa8\x56\x48\xcb\xcf\x2b\xd1\x2d\xce\xac\x4a\xb5\x52\x30\x34\x32\x4e\xcd\xb5\x56\xa8\xe5\x02\x00\x0c\x97\x72\x35\x18\x00\x00\x00"sv;

// p { font-size: 123em; }, zlibbed.
constexpr auto kZlibbedCss =
        "\x78\x5e\x2b\x50\xa8\x56\x48\xcb\xcf\x2b\xd1\x2d\xce\xac\x4a\xb5\x52\x30\x34\x32\x4e\xcd\xb5\x56\xa8\xe5\x02\x00\x63\xc3\x07\x6f"sv;

} // namespace

int main() {
    etest::Suite s{};
    s.add_test("zlib", [](etest::IActions &a) {
        a.expect(!zlib_decode({}, ZlibMode::Zlib).has_value());
        a.expect(!zlib_decode(as_bytes(kGzippedCss), ZlibMode::Zlib).has_value());

        auto res = zlib_decode(as_bytes(kZlibbedCss), ZlibMode::Zlib);
        a.expect(std::ranges::equal(res.value(), as_bytes(kExpected)));
    });

    s.add_test("gzip", [](etest::IActions &a) {
        a.expect(!zlib_decode({}, ZlibMode::Gzip).has_value());
        a.expect(!zlib_decode(as_bytes(kZlibbedCss), ZlibMode::Gzip).has_value());

        auto res = zlib_decode(as_bytes(kGzippedCss), ZlibMode::Gzip);
        a.expect(std::ranges::equal(res.value(), as_bytes(kExpected)));
    });

    return s.run();
}
