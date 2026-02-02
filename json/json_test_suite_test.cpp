// SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "json/json.h"

#include "etest/etest2.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <span>
#include <string>

namespace {

enum class Expected : std::uint8_t {
    DontCare,
    Parses,
    Fails,
};

// This is the convention /test_parsing/ in https://github.com/nst/JSONTestSuite/ follows.
std::optional<Expected> expectation_from_file_name(std::filesystem::path const &path) {
    auto filename = path.filename().u8string();

    if (filename.starts_with(u8"y_")) {
        return Expected::Parses;
    }

    if (filename.starts_with(u8"n_")) {
        return Expected::Fails;
    }

    if (filename.starts_with(u8"i_")) {
        return Expected::DontCare;
    }

    return std::nullopt;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        char const *program_name = argv[0] != nullptr ? argv[0] : "<bin>";
        std::cerr << "Usage: " << program_name << " <file1.json> [<file2.json>]...\n";
        return 1;
    }

    auto args = std::span{argv + 1, static_cast<std::size_t>(argc - 1)};

    etest::Suite s{};

    for (auto const &file_path : args) {
        std::filesystem::path path{file_path};
        s.add_test(path.filename().string(), [path, file_path](etest::IActions &a) {
            auto expectation = expectation_from_file_name(path);
            if (!expectation.has_value()) {
                a.expect(false, std::format("Could not determine expectation from file name: {}", file_path));
                return;
            }

            std::ifstream fs{path};
            if (!fs) {
                a.expect(false, std::format("Failed to open {} for reading", file_path));
                return;
            }

            std::string file_contents{std::istreambuf_iterator<char>(fs), std::istreambuf_iterator<char>()};

            auto result = json::parse(file_contents);
            switch (*expectation) {
                case Expected::Parses:
                    a.expect(result.has_value(), std::format("Expected {} to parse successfully", file_path));
                    break;
                case Expected::Fails:
                    a.expect(!result.has_value(), std::format("Expected {} to fail parsing", file_path));
                    break;
                case Expected::DontCare:
                    break;
            }
        });
    }

    return s.run();
}
