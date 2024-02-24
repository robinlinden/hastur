// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/instructions.h"

#include "wasm/byte_code_parser.h"
#include "wasm/leb128.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <istream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace wasm::instructions {

// clangd (16) crashes if this is = default even though though it's allowed and
// clang has alledegly implemented it starting with Clang 14:
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2085r0.html
// https://clang.llvm.org/cxx_status.html
bool Block::operator==(Block const &b) const {
    return b.type == type && b.instructions == instructions;
}

bool Loop::operator==(Loop const &l) const {
    return l.type == type && l.instructions == instructions;
}

std::optional<BlockType> BlockType::parse(std::istream &is) {
    std::uint8_t type{};
    if (!is.read(reinterpret_cast<char *>(&type), sizeof(type))) {
        return std::nullopt;
    }

    constexpr std::uint8_t kEmptyTag = 0x40;
    if (type == kEmptyTag) {
        return BlockType{{BlockType::Empty{}}};
    }

    std::stringstream ss{std::string{static_cast<char>(type)}};
    auto value_type = ByteCodeParser::parse_value_type(ss);
    if (value_type) {
        return BlockType{{*std::move(value_type)}};
    }

    std::cerr << "Unhandled BlockType\n";
    return std::nullopt;
}

std::optional<MemArg> MemArg::parse(std::istream &is) {
    auto a = wasm::Leb128<std::uint32_t>::decode_from(is);
    if (!a) {
        return std::nullopt;
    }

    auto o = wasm::Leb128<std::uint32_t>::decode_from(is);
    if (!o) {
        return std::nullopt;
    }

    return MemArg{.align = *std::move(a), .offset = *std::move(o)};
}

std::optional<std::vector<Instruction>> parse(std::istream &is) {
    std::vector<Instruction> instructions{};

    while (true) {
        std::uint8_t opcode{};
        if (!is.read(reinterpret_cast<char *>(&opcode), sizeof(opcode))) {
            return std::nullopt;
        }

        switch (opcode) {
            case Block::kOpcode: {
                auto type = BlockType::parse(is);
                if (!type) {
                    return std::nullopt;
                }

                auto block_instructions = parse(is);
                if (!block_instructions) {
                    return std::nullopt;
                }

                instructions.emplace_back(Block{*std::move(type), *std::move(block_instructions)});
                break;
            }
            case Loop::kOpcode: {
                auto type = BlockType::parse(is);
                if (!type) {
                    return std::nullopt;
                }

                auto block_instructions = parse(is);
                if (!block_instructions) {
                    return std::nullopt;
                }

                instructions.emplace_back(Loop{*std::move(type), *std::move(block_instructions)});
                break;
            }
            case BreakIf::kOpcode: {
                auto value = wasm::Leb128<std::uint32_t>::decode_from(is);
                if (!value) {
                    return std::nullopt;
                }
                instructions.emplace_back(BreakIf{*value});
                break;
            }
            case Return::kOpcode:
                instructions.emplace_back(Return{});
                break;
            case End::kOpcode:
                return instructions;
            case I32Const::kOpcode: {
                auto value = wasm::Leb128<std::int32_t>::decode_from(is);
                if (!value) {
                    return std::nullopt;
                }
                instructions.emplace_back(I32Const{*value});
                break;
            }
            case I32LessThanSigned::kOpcode:
                instructions.emplace_back(I32LessThanSigned{});
                break;
            case I32Add::kOpcode:
                instructions.emplace_back(I32Add{});
                break;
            case I32Sub::kOpcode:
                instructions.emplace_back(I32Sub{});
                break;
            case LocalGet::kOpcode: {
                auto value = wasm::Leb128<std::uint32_t>::decode_from(is);
                if (!value) {
                    return std::nullopt;
                }
                instructions.emplace_back(LocalGet{*value});
                break;
            }
            case LocalSet::kOpcode: {
                auto value = wasm::Leb128<std::uint32_t>::decode_from(is);
                if (!value) {
                    return std::nullopt;
                }
                instructions.emplace_back(LocalSet{*value});
                break;
            }
            case LocalTee::kOpcode: {
                auto value = wasm::Leb128<std::uint32_t>::decode_from(is);
                if (!value) {
                    return std::nullopt;
                }
                instructions.emplace_back(LocalTee{*value});
                break;
            }
            case I32Load::kOpcode: {
                auto arg = MemArg::parse(is);
                if (!arg) {
                    return std::nullopt;
                }

                instructions.emplace_back(I32Load{*std::move(arg)});
                break;
            }
            default:
                std::cerr << "Unhandled opcode 0x" << std::setw(2) << std::setfill('0') << std::hex << +opcode << '\n';
                return std::nullopt;
        }
    }
}

} // namespace wasm::instructions
