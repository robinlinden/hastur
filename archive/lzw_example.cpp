// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/lzw.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << (argv[0] ? argv[0] : "<bin>") << ' ' << "<compressed_lzw_file.Z>\n";
        return 1;
    }

    auto fs = std::ifstream{argv[1], std::fstream::in | std::fstream::binary};
    if (!fs) {
        std::cerr << "Unable to open " << argv[1] << " for reading\n";
        return 1;
    }

    std::vector<std::uint8_t> lzw_data{std::istreambuf_iterator<char>(fs), {}};
    auto decompressed = archive::Lzw::decompress(lzw_data);
    if (!decompressed) {
        std::cerr << "Unable to decompress " << argv[1] << '\n';
        return 1;
    }

    std::cout << *decompressed;
}
