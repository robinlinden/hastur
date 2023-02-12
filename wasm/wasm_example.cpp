// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/wasm.h"

#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << (argv[0] ? argv[0] : "<bin>") << ' ' << "<wasm_file>\n";
        return 1;
    }

    auto fs = std::ifstream{argv[1], std::fstream::in | std::fstream::binary};
    if (!fs) {
        std::cerr << "Unable to open " << argv[1] << " for reading\n";
        return 1;
    }

    auto module = wasm::Module::parse_from(fs);
    if (!module) {
        std::cerr << "Unable to parse " << argv[1] << " as a wasm module\n";
        return 1;
    }

    for (auto const &section : module->sections) {
        std::cout << static_cast<int>(section.id) << ": " << section.content.size() << '\n';
    }
}
