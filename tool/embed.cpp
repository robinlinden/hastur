// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/arg_parser.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

int main(int argc, char **argv) {
    std::string name;
    std::string input_file;

    if (auto res = util::ArgParser{}.argument("--name", name).positional(input_file).parse(argc, argv); !res) {
        std::cerr << "Error: " << res.error().message << '\n';
        return 1;
    }

    if (input_file.empty() || name.empty()) {
        std::string_view program_name = argv[0] != nullptr ? argv[0] : "<bin>";
        std::cerr << "Usage: " << program_name << " --name <name> <input_file>\n";
        return 1;
    }

    std::ifstream file(input_file, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file '" << input_file << "'\n";
        return 1;
    }

    std::error_code ec{};
    auto file_size = std::filesystem::file_size(input_file, ec);
    if (ec) {
        std::cerr << "Error: Could not get file size of '" << input_file << "': " << ec.message() << '\n';
        return 1;
    }

    std::cout << "#ifndef HST_GENERATED_" << name << "_H\n";
    std::cout << "#define HST_GENERATED_" << name << "_H\n";

    std::cout << "#include <array>\n";
    std::cout << "#include <string_view>\n";
    std::cout << "constexpr auto " << name << "Bytes = std::array<char, " << file_size << ">{\n";
    std::array<char, 1024> data{};
    while (file.read(data.data(), data.size()) || file.gcount() > 0) {
        std::string_view chunk{data.data(), static_cast<std::size_t>(file.gcount())};
        std::size_t count{};
        for (unsigned char c : chunk) {
            std::cout << "'\\x" << std::hex << std::setw(2) << std::setfill('0') << int{c} << "',";
            count++;
            if (count % 32 == 0) {
                std::cout << '\n';
            }
        }
    }
    std::cout << "};\n";

    std::cout << "constexpr auto " << name << " = std::string_view{" << name << "Bytes.data(), " << name
              << "Bytes.size()};\n";
    std::cout << "#endif // HST_GENERATED_" << name << "_H\n";
}
