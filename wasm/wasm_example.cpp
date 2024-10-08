// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/byte_code_parser.h"
#include "wasm/serialize.h"
#include "wasm/types.h"
#include "wasm/wasm.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string_view>
#include <variant>

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << (argv[0] != nullptr ? argv[0] : "<bin>") << ' ' << "<wasm_file>\n";
        return 1;
    }

    auto fs = std::ifstream{argv[1], std::fstream::in | std::fstream::binary};
    if (!fs) {
        std::cerr << "Unable to open " << argv[1] << " for reading\n";
        return 1;
    }

    auto module = wasm::ByteCodeParser::parse_module(fs);
    if (!module) {
        std::cerr << "Unable to parse " << argv[1] << " as a wasm module: " << to_string(module.error()) << '\n';
        return 1;
    }

    if (auto const &type_section = module->type_section) {
        std::cout << "\n# Types\n";
        // Prints a list of wasm::ValueType separated by commas.
        // https://en.cppreference.com/w/cpp/experimental/ostream_joiner soon, I hope.
        auto print_values = [](auto const &values) {
            if (!values.empty()) {
                for (std::size_t i = 0; i < values.size() - 1; ++i) {
                    std::cout << to_string(values[i]) << ',';
                }

                std::cout << to_string(values.back());
            }
        };

        for (auto const &type : type_section->types) {
            std::cout << '(';
            print_values(type.parameters);
            std::cout << ") -> (";
            print_values(type.results);
            std::cout << ")\n";
        }
    }

    if (auto const &import_section = module->import_section) {
        struct ImportStringifier {
            std::string_view operator()(wasm::TypeIdx) const { return "func"; }
            std::string_view operator()(wasm::TableType const &) const { return "table"; }
            std::string_view operator()(wasm::MemType const &) const { return "mem"; }
            std::string_view operator()(wasm::GlobalType const &) const { return "global"; }
        };

        std::cout << "\n# Imports\n";
        for (auto const &i : import_section->imports) {
            std::cout << i.module << '.' << i.name << ": " << std::visit(ImportStringifier{}, i.description) << '\n';
        }
    }

    if (auto const &function_section = module->function_section) {
        std::cout << "\n# Function idx -> type idx\n";
        for (std::size_t i = 0; i < function_section->type_indices.size(); ++i) {
            std::cout << i << " -> " << function_section->type_indices[i] << '\n';
        }
    }

    if (auto const &export_section = module->export_section) {
        std::cout << "\n# Exports\n";
        for (auto const &e : export_section->exports) {
            std::cout << e.name << ": " << static_cast<int>(e.type) << ':' << e.index << '\n';
        }
    }

    if (auto const &s = module->code_section) {
        std::cout << "\n# Code\n";
        for (auto const &e : s->entries) {
            std::cout << e.code.size() << " instruction(s), " << e.locals.size() << " locals";
            for (auto const &local : e.locals) {
                std::cout << " (" << to_string(local.type) << ": " << local.count << ')';
            }
            std::cout << '\n';
        }
    }
}
