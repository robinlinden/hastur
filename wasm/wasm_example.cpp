// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/byte_code_parser.h"
#include "wasm/instructions.h"
#include "wasm/types.h"
#include "wasm/wasm.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <type_traits>

namespace wasm {
std::ostream &operator<<(std::ostream &, wasm::ValueType);
std::ostream &operator<<(std::ostream &os, wasm::ValueType type) {
    switch (type.kind) {
        case ValueType::Int32:
            os << "i32";
            break;
        case ValueType::Int64:
            os << "i64";
            break;
        case ValueType::Float32:
            os << "f32";
            break;
        case ValueType::Float64:
            os << "f64";
            break;
        case ValueType::Vector128:
            os << "v128";
            break;
        case ValueType::FunctionReference:
            os << "funcref";
            break;
        case ValueType::ExternReference:
            os << "externref";
            break;
        default:
            std::abort();
    }
    return os;
}
} // namespace wasm

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
        std::cerr << "Unable to parse " << argv[1] << " as a wasm module: "
                  << static_cast<std::underlying_type_t<wasm::ModuleParseError>>(module.error()) << '\n';
        return 1;
    }

    if (auto const &type_section = module->type_section) {
        std::cout << "\n# Types\n";
        // Prints a list of wasm::ValueType separated by commas.
        // https://en.cppreference.com/w/cpp/experimental/ostream_joiner soon, I hope.
        auto print_values = [](auto const &values) {
            if (!values.empty()) {
                std::copy_n(begin(values), size(values) - 1, std::ostream_iterator<wasm::ValueType>(std::cout, ","));
                std::copy_n(end(values) - 1, 1, std::ostream_iterator<wasm::ValueType>(std::cout));
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
            std::cout << e.code.size() << "B code, " << e.locals.size() << " locals";
            for (auto const &local : e.locals) {
                std::cout << " (" << local.type << ": " << local.count << ')';
            }
            std::cout << '\n';

            std::stringstream ss{std::string{reinterpret_cast<char const *>(e.code.data()), e.code.size()}};
            auto instructions = wasm::instructions::parse(ss);
            if (!instructions) {
                std::cout << "Unable to parse instructions!\n";
            } else {
                std::cout << "Entry has " << instructions->size() << " instruction\n";
            }
        }
    }
}
