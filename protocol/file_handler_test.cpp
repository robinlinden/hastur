// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/file_handler.h"

#include "protocol/response.h"

#include "etest/etest2.h"
#include "uri/uri.h"

#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>

namespace fs = std::filesystem;

int main() {
    // https://bazel.build/reference/test-encyclopedia#test-interaction-filesystem
    auto *const bazel_tmp_dir = std::getenv("TEST_TMPDIR"); // NOLINT(concurrency-mt-unsafe)
    if (bazel_tmp_dir == nullptr) {
        std::cerr << "TEST_TMPDIR must be set\n";
        return 1;
    }

    auto const tmp_dir = fs::path{bazel_tmp_dir};

    etest::Suite s;

    s.add_test("uri pointing to non-existent file", [](etest::IActions &a) {
        protocol::FileHandler handler;
        auto res = handler.handle(uri::Uri::parse("file:///this/file/does/definitely/not/exist.hastur").value());
        a.expect_eq(res.error(), protocol::Error{protocol::ErrorCode::Unresolved});
    });

#ifdef _WIN32
    // TOOD(robinlinden): Test case that's not Windows-specific.
    s.add_test("invalid file", [](etest::IActions &a) {
        protocol::FileHandler handler;
        auto res = handler.handle(uri::Uri::parse("NUL").value());
        a.expect_eq(res.error(), protocol::Error{protocol::ErrorCode::InvalidResponse});
    });
#endif

    s.add_test("uri pointing to a folder", [&](etest::IActions &a) {
        protocol::FileHandler handler;
        auto res = handler.handle(uri::Uri::parse(std::format("file://{}", tmp_dir.generic_string())).value());
        a.expect_eq(res.error(), protocol::Error{protocol::ErrorCode::InvalidResponse});
    });

    s.add_test("uri pointing to a regular file", [&](etest::IActions &a) {
        auto tmp_dst = tmp_dir / "hastur-uri-pointing-to-a-regular-file-test";
        {
            std::ofstream tmp_file{tmp_dst};
            a.require(!tmp_file.fail());
            a.require(bool{tmp_file << "hello!"});
        }

        protocol::FileHandler handler;
        auto res = handler.handle(uri::Uri::parse(std::format("file://{}", tmp_dst.generic_string())).value());
        a.expect_eq(res, protocol::Response{{}, {}, "hello!"});
    });

    return s.run();
}
