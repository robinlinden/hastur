// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/file_handler.h"

#include "protocol/response.h"

#include "etest/etest.h"
#include "uri/uri.h"

#include <fmt/core.h>

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <utility>

namespace fs = std::filesystem;
using etest::expect_eq;
using etest::require;

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
    etest::test("uri pointing to non-existent file", [] {
        protocol::FileHandler handler;
        auto res = handler.handle(uri::Uri::parse("file:///this/file/does/definitely/not/exist.hastur"));
        expect_eq(res, protocol::Response{protocol::Error::Unresolved});
    });

    etest::test("uri pointing to a folder", [] {
        auto tmp_dir = fs::temp_directory_path();

        protocol::FileHandler handler;
        auto res = handler.handle(uri::Uri::parse(fmt::format("file://{}", tmp_dir.generic_string())));
        expect_eq(res, protocol::Response{protocol::Error::InvalidResponse});
    });

    etest::test("uri pointing to a regular file", [] {
        std::random_device rng;
        auto tmp_dst = fs::temp_directory_path() / fmt::format("hastur-uri-pointing-to-a-regular-file-test.{}", rng());

        auto tmp_file = TmpFile::create(std::move(tmp_dst));
        require(tmp_file.has_value());
        require(bool{tmp_file->fstream() << "hello!" << std::flush});

        protocol::FileHandler handler;
        auto res = handler.handle(uri::Uri::parse(fmt::format("file://{}", tmp_file->path().generic_string())));
        expect_eq(res, protocol::Response{protocol::Error::Ok, {}, {}, "hello!"});
    });

    return etest::run_all_tests();
}
