// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/file_handler.h"

#include "protocol/response.h"

#include "etest/etest2.h"
#include "uri/uri.h"

#include <cerrno>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <utility>

namespace fs = std::filesystem;

namespace {

class TmpFile {
public:
    explicit TmpFile(fs::path location, std::fstream file) : location_{std::move(location)}, file_{std::move(file)} {}

    static std::optional<TmpFile> create(fs::path location) {
        if (fs::exists(location)) {
            std::cerr << "File already exists at " << location.generic_string() << '\n';
            return std::nullopt;
        }

        std::fstream file{location, std::fstream::in | std::fstream::out | std::fstream::trunc};
        if (!file) {
            std::cerr << "Unable to create file at " << location.generic_string() << " (" << errno << ")\n";
            return std::nullopt;
        }

        return std::optional<TmpFile>{std::in_place_t{}, std::move(location), std::move(file)};
    }

    fs::path const &path() const { return location_; }
    std::fstream &fstream() { return file_; }

    ~TmpFile() {
        file_.close();
        fs::remove(location_);
    }

private:
    fs::path location_;
    std::fstream file_;
};

} // namespace

int main() {
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

    s.add_test("uri pointing to a folder", [](etest::IActions &a) {
        auto tmp_dir = fs::temp_directory_path();

        protocol::FileHandler handler;
        auto res = handler.handle(uri::Uri::parse(std::format("file://{}", tmp_dir.generic_string())).value());
        a.expect_eq(res.error(), protocol::Error{protocol::ErrorCode::InvalidResponse});
    });

    s.add_test("uri pointing to a regular file", [](etest::IActions &a) {
        std::random_device rng;
        auto tmp_dst = fs::temp_directory_path() / std::format("hastur-uri-pointing-to-a-regular-file-test.{}", rng());

        auto tmp_file = TmpFile::create(std::move(tmp_dst));
        a.require(tmp_file.has_value());
        a.require(bool{tmp_file->fstream() << "hello!" << std::flush});

        protocol::FileHandler handler;
        auto res = handler.handle(uri::Uri::parse(std::format("file://{}", tmp_file->path().generic_string())).value());
        a.expect_eq(res, protocol::Response{{}, {}, "hello!"});
    });

    return s.run();
}
