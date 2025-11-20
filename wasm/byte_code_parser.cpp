// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/byte_code_parser.h"

#include "wasm/instructions.h"
#include "wasm/leb128.h"
#include "wasm/types.h"
#include "wasm/wasm.h"

#include <tl/expected.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <istream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace wasm {
namespace {

// Number 100% made up. We'll definitely have to adjust this.
constexpr std::size_t kMaxSequenceSize = UINT16_MAX;

constexpr int kMagicSize = 4;
constexpr int kVersionSize = 4;

std::optional<std::vector<instructions::Instruction>> parse_instructions(std::istream &);

template<typename T>
std::optional<std::vector<T>> parse_vector(std::istream &);
template<typename T>
std::optional<std::vector<T>> parse_vector(std::istream &&);

template<typename T>
std::optional<T> parse(std::istream &) = delete;
template<typename T>
std::optional<T> parse(std::istream &&is) {
    return parse<T>(is);
}

// https://webassembly.github.io/spec/core/binary/values.html#names
template<>
std::optional<std::string> parse(std::istream &is) {
    auto length = Leb128<std::uint32_t>::decode_from(is);
    if (!length || *length > kMaxSequenceSize) {
        return std::nullopt;
    }

    std::string str;
    str.resize(*length);
    // TODO(robinlinden): Handle non-ascii. This needs to be valid UTF-8.
    if (!is.read(str.data(), *length) || std::ranges::any_of(str, [](unsigned char c) { return c > 0x7f; })) {
        return std::nullopt;
    }

    return str;
}

template<>
std::optional<std::uint32_t> parse(std::istream &is) {
    auto v = Leb128<std::uint32_t>::decode_from(is);
    return v ? std::optional{*v} : std::nullopt;
}

template<>
std::optional<std::byte> parse(std::istream &is) {
    std::byte b{};
    if (!is.read(reinterpret_cast<char *>(&b), sizeof(b))) {
        return std::nullopt;
    }
    return b;
}

// https://webassembly.github.io/spec/core/binary/types.html
template<>
std::optional<ValueType> parse(std::istream &is) {
    std::uint8_t byte{};
    if (!is.read(reinterpret_cast<char *>(&byte), sizeof(byte))) {
        return std::nullopt;
    }

    switch (byte) {
        case 0x7f:
            return ValueType::Int32;
        case 0x7e:
            return ValueType::Int64;
        case 0x7d:
            return ValueType::Float32;
        case 0x7c:
            return ValueType::Float64;
        case 0x7b:
            return ValueType::Vector128;
        case 0x70:
            return ValueType::FunctionReference;
        case 0x6f:
            return ValueType::ExternReference;
        default:
            return std::nullopt;
    }
}

template<>
std::optional<Limits> parse(std::istream &is) {
    std::uint8_t has_max{};
    if (!is.read(reinterpret_cast<char *>(&has_max), sizeof(has_max)) || has_max > 1) {
        return std::nullopt;
    }

    auto min = Leb128<std::uint32_t>::decode_from(is);
    if (!min) {
        return std::nullopt;
    }

    if (has_max == 0) {
        return Limits{.min = *min};
    }

    auto max = Leb128<std::uint32_t>::decode_from(is);
    if (!max) {
        return std::nullopt;
    }

    return Limits{.min = *min, .max = *max};
}

template<>
std::optional<GlobalType> parse(std::istream &is) {
    auto valtype = parse<ValueType>(is);
    if (!valtype) {
        return std::nullopt;
    }

    std::uint8_t mut{};
    if (!is.read(reinterpret_cast<char *>(&mut), sizeof(mut)) || mut > 1) {
        return std::nullopt;
    }

    return GlobalType{
            .type = *valtype,
            .mutability = mut == 0 ? GlobalType::Mutability::Const : GlobalType::Mutability::Var,
    };
}

template<>
std::optional<Global> parse(std::istream &is) {
    auto type = parse<GlobalType>(is);
    if (!type) {
        return std::nullopt;
    }

    auto init = parse_instructions(is);
    if (!init) {
        return std::nullopt;
    }

    return Global{
            .type = *type,
            .init = *std::move(init),
    };
}

// https://webassembly.github.io/spec/core/binary/types.html#function-types
template<>
std::optional<FunctionType> parse(std::istream &is) {
    std::uint8_t magic{};
    if (!is.read(reinterpret_cast<char *>(&magic), sizeof(magic)) || magic != 0x60) {
        return std::nullopt;
    }

    auto parameters = parse_vector<ValueType>(is);
    if (!parameters) {
        return std::nullopt;
    }

    auto results = parse_vector<ValueType>(is);
    if (!results) {
        return std::nullopt;
    }

    return FunctionType{
            .parameters = *std::move(parameters),
            .results = *std::move(results),
    };
}

template<>
std::optional<TableType> parse(std::istream &is) {
    auto element_type = parse<ValueType>(is);
    if (!element_type || (element_type != ValueType::FunctionReference && element_type != ValueType::ExternReference)) {
        return std::nullopt;
    }

    auto limits = parse<Limits>(is);
    if (!limits) {
        return std::nullopt;
    }

    return TableType{.element_type = *element_type, .limits = *limits};
}

// https://webassembly.github.io/spec/core/binary/modules.html#binary-exportsec
template<>
std::optional<Export> parse(std::istream &is) {
    auto name = parse<std::string>(is);
    if (!name) {
        return std::nullopt;
    }

    std::uint8_t type{};
    if (!is.read(reinterpret_cast<char *>(&type), sizeof(type)) || type > 0x03) {
        return std::nullopt;
    }

    auto index = Leb128<std::uint32_t>::decode_from(is);
    if (!index) {
        return std::nullopt;
    }

    return Export{
            .name = *std::move(name),
            .type = static_cast<Export::Type>(type),
            .index = *index,
    };
}

// https://webassembly.github.io/spec/core/binary/modules.html#binary-codesec
template<>
std::optional<CodeEntry::Local> parse(std::istream &is) {
    auto count = Leb128<std::uint32_t>::decode_from(is);
    if (!count) {
        return std::nullopt;
    }

    auto type = parse<ValueType>(is);
    if (!type) {
        return std::nullopt;
    }

    return CodeEntry::Local{
            .count = *count,
            .type = *type,
    };
}

// https://webassembly.github.io/spec/core/binary/modules.html#binary-codesec
template<>
std::optional<CodeEntry> parse(std::istream &is) {
    auto size = Leb128<std::uint32_t>::decode_from(is);
    if (!size) {
        return std::nullopt;
    }

    auto locals = parse_vector<CodeEntry::Local>(is);
    if (!locals) {
        return std::nullopt;
    }

    auto instructions = parse_instructions(is);
    if (!instructions) {
        return std::nullopt;
    }

    return CodeEntry{
            .code = *std::move(instructions),
            .locals = *std::move(locals),
    };
}

// https://webassembly.github.io/spec/core/binary/modules.html#binary-codesec
template<>
std::optional<DataSection::Data> parse(std::istream &is) {
    auto type = Leb128<std::uint32_t>::decode_from(is);
    if (!type) {
        return std::nullopt;
    }

    static constexpr std::uint32_t kActiveDataTag = 0;
    static constexpr std::uint32_t kPassiveDataTag = 1;
    static constexpr std::uint32_t kActiveDataWithMemIdxTag = 2;

    if (*type == kPassiveDataTag) {
        // TODO(robinlinden): We can read more than 1 byte at a time to speed this up.
        auto init = parse_vector<std::byte>(is);
        if (!init) {
            return std::nullopt;
        }

        return DataSection::Data{DataSection::PassiveData{.data = *std::move(init)}};
    }

    std::uint32_t memory_idx{};
    if (*type == kActiveDataWithMemIdxTag) {
        auto idx = Leb128<std::uint32_t>::decode_from(is);
        if (!idx) {
            return std::nullopt;
        }

        memory_idx = *idx;
    } else if (*type != kActiveDataTag) {
        return std::nullopt;
    }

    auto offset = parse_instructions(is);
    if (!offset) {
        return std::nullopt;
    }

    auto init = parse_vector<std::byte>(is);
    if (!init) {
        return std::nullopt;
    }

    return DataSection::Data{DataSection::ActiveData{
            .memory_idx = memory_idx,
            .offset = *std::move(offset),
            .data = *std::move(init),
    }};
}

// https://webassembly.github.io/spec/core/binary/modules.html#binary-import
template<>
std::optional<Import> parse(std::istream &is) {
    auto module = parse<std::string>(is);
    if (!module) {
        return std::nullopt;
    }

    auto name = parse<std::string>(is);
    if (!name) {
        return std::nullopt;
    }

    std::uint8_t kind{};
    if (!is.read(reinterpret_cast<char *>(&kind), sizeof(kind))) {
        return std::nullopt;
    }

    std::optional<Import::Description> desc;
    switch (kind) {
        case 0x00:
            desc = parse<TypeIdx>(is);
            break;
        case 0x01:
            desc = parse<TableType>(is);
            break;
        case 0x02:
            desc = parse<MemType>(is);
            break;
        case 0x03:
            desc = parse<GlobalType>(is);
            break;
        default:
            break;
    }

    if (!desc) {
        return std::nullopt;
    }

    return Import{
            .module = *std::move(module),
            .name = *std::move(name),
            .description = *desc,
    };
}

template<>
std::optional<instructions::BlockType> parse(std::istream &is) {
    using namespace instructions;
    std::uint8_t type{};
    if (!is.read(reinterpret_cast<char *>(&type), sizeof(type))) {
        return std::nullopt;
    }

    constexpr std::uint8_t kEmptyTag = 0x40;
    if (type == kEmptyTag) {
        return BlockType{{BlockType::Empty{}}};
    }

    std::stringstream ss{std::string{static_cast<char>(type)}};
    auto value_type = parse<ValueType>(ss);
    if (value_type) {
        return BlockType{{*value_type}};
    }

    std::cerr << "Unhandled BlockType\n";
    return std::nullopt;
}

template<>
std::optional<instructions::MemArg> parse(std::istream &is) {
    using namespace instructions;
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

// https://webassembly.github.io/spec/core/binary/conventions.html#vectors
template<typename T>
std::optional<std::vector<T>> parse_vector(std::istream &is) {
    auto item_count = Leb128<std::uint32_t>::decode_from(is);
    if (!item_count || *item_count > kMaxSequenceSize) {
        return std::nullopt;
    }

    std::vector<T> items;
    items.reserve(*item_count);
    for (std::uint32_t i = 0; i < *item_count; ++i) {
        auto item = parse<T>(is);
        if (!item) {
            return std::nullopt;
        }

        items.push_back(std::move(item).value());
    }

    return items;
}

template<typename T>
std::optional<std::vector<T>> parse_vector(std::istream &&is) {
    return parse_vector<T>(is);
}

std::optional<TypeSection> parse_type_section(std::istream &is) {
    if (auto maybe_types = parse_vector<FunctionType>(is)) {
        return TypeSection{.types = *std::move(maybe_types)};
    }

    return std::nullopt;
}

std::optional<ImportSection> parse_import_section(std::istream &is) {
    if (auto maybe_imports = parse_vector<Import>(is)) {
        return ImportSection{.imports = *std::move(maybe_imports)};
    }

    return std::nullopt;
}

std::optional<FunctionSection> parse_function_section(std::istream &is) {
    if (auto maybe_type_indices = parse_vector<TypeIdx>(is)) {
        return FunctionSection{.type_indices = *std::move(maybe_type_indices)};
    }

    return std::nullopt;
}

std::optional<TableSection> parse_table_section(std::istream &is) {
    if (auto maybe_tables = parse_vector<TableType>(is)) {
        return TableSection{*std::move(maybe_tables)};
    }

    return std::nullopt;
}

std::optional<MemorySection> parse_memory_section(std::istream &is) {
    if (auto maybe_memories = parse_vector<MemType>(is)) {
        return MemorySection{*std::move(maybe_memories)};
    }

    return std::nullopt;
}

std::optional<GlobalSection> parse_global_section(std::istream &is) {
    if (auto maybe_globals = parse_vector<Global>(is)) {
        return GlobalSection{*std::move(maybe_globals)};
    }

    return std::nullopt;
}

std::optional<ExportSection> parse_export_section(std::istream &is) {
    if (auto maybe_exports = parse_vector<Export>(is)) {
        return ExportSection{.exports = std::move(maybe_exports).value()};
    }

    return std::nullopt;
}

std::optional<StartSection> parse_start_section(std::istream &is) {
    if (auto maybe_start = parse<FuncIdx>(is)) {
        return StartSection{.start = *maybe_start};
    }

    return std::nullopt;
}

std::optional<CodeSection> parse_code_section(std::istream &is) {
    if (auto code_entries = parse_vector<CodeEntry>(is)) {
        return CodeSection{.entries = *std::move(code_entries)};
    }

    return std::nullopt;
}

std::optional<DataSection> parse_data_section(std::istream &is) {
    if (auto data_entries = parse_vector<DataSection::Data>(is)) {
        return DataSection{.data = *std::move(data_entries)};
    }

    return std::nullopt;
}

std::optional<std::vector<instructions::Instruction>> parse_instructions(std::istream &is) {
    using namespace instructions;
    std::vector<Instruction> instructions;

    // If an End-opcode is encountered when nesting == 0, we're done.
    int nesting = 0;

    while (true) {
        std::uint8_t opcode{};
        if (!is.read(reinterpret_cast<char *>(&opcode), sizeof(opcode))) {
            return std::nullopt;
        }

        switch (opcode) {
            case Select::kOpcode:
                instructions.emplace_back(Select{});
                break;
            case Block::kOpcode: {
                auto type = parse<BlockType>(is);
                if (!type) {
                    return std::nullopt;
                }

                instructions.emplace_back(Block{*type});
                ++nesting;
                break;
            }
            case Loop::kOpcode: {
                auto type = parse<BlockType>(is);
                if (!type) {
                    return std::nullopt;
                }

                instructions.emplace_back(Loop{*type});
                ++nesting;
                break;
            }
            case Branch::kOpcode: {
                auto value = wasm::Leb128<std::uint32_t>::decode_from(is);
                if (!value) {
                    return std::nullopt;
                }
                instructions.emplace_back(Branch{*value});
                break;
            }
            case BranchIf::kOpcode: {
                auto value = wasm::Leb128<std::uint32_t>::decode_from(is);
                if (!value) {
                    return std::nullopt;
                }
                instructions.emplace_back(BranchIf{*value});
                break;
            }
            case Call::kOpcode: {
                auto value = wasm::Leb128<std::uint32_t>::decode_from(is);
                if (!value) {
                    return std::nullopt;
                }
                instructions.emplace_back(Call{*value});
                break;
            }
            case Return::kOpcode:
                instructions.emplace_back(Return{});
                break;
            case End::kOpcode:
                instructions.emplace_back(End{});
                if (nesting == 0) {
                    return instructions;
                }

                --nesting;
                break;
            case I32Const::kOpcode: {
                auto value = wasm::Leb128<std::int32_t>::decode_from(is);
                if (!value) {
                    return std::nullopt;
                }
                instructions.emplace_back(I32Const{*value});
                break;
            }
            case I32EqualZero::kOpcode:
                instructions.emplace_back(I32EqualZero{});
                break;
            case I32Equal::kOpcode:
                instructions.emplace_back(I32Equal{});
                break;
            case I32NotEqual::kOpcode:
                instructions.emplace_back(I32NotEqual{});
                break;
            case I32LessThanSigned::kOpcode:
                instructions.emplace_back(I32LessThanSigned{});
                break;
            case I32LessThanUnsigned::kOpcode:
                instructions.emplace_back(I32LessThanUnsigned{});
                break;
            case I32GreaterThanSigned::kOpcode:
                instructions.emplace_back(I32GreaterThanSigned{});
                break;
            case I32GreaterThanUnsigned::kOpcode:
                instructions.emplace_back(I32GreaterThanUnsigned{});
                break;
            case I32LessThanEqualSigned::kOpcode:
                instructions.emplace_back(I32LessThanEqualSigned{});
                break;
            case I32LessThanEqualUnsigned::kOpcode:
                instructions.emplace_back(I32LessThanEqualUnsigned{});
                break;
            case I32GreaterThanEqualSigned::kOpcode:
                instructions.emplace_back(I32GreaterThanEqualSigned{});
                break;
            case I32GreaterThanEqualUnsigned::kOpcode:
                instructions.emplace_back(I32GreaterThanEqualUnsigned{});
                break;
            case I32CountLeadingZeros::kOpcode:
                instructions.emplace_back(I32CountLeadingZeros{});
                break;
            case I32CountTrailingZeros::kOpcode:
                instructions.emplace_back(I32CountTrailingZeros{});
                break;
            case I32PopulationCount::kOpcode:
                instructions.emplace_back(I32PopulationCount{});
                break;
            case I32Add::kOpcode:
                instructions.emplace_back(I32Add{});
                break;
            case I32Subtract::kOpcode:
                instructions.emplace_back(I32Subtract{});
                break;
            case I32Multiply::kOpcode:
                instructions.emplace_back(I32Multiply{});
                break;
            case I32DivideSigned::kOpcode:
                instructions.emplace_back(I32DivideSigned{});
                break;
            case I32DivideUnsigned::kOpcode:
                instructions.emplace_back(I32DivideUnsigned{});
                break;
            case I32RemainderSigned::kOpcode:
                instructions.emplace_back(I32RemainderSigned{});
                break;
            case I32RemainderUnsigned::kOpcode:
                instructions.emplace_back(I32RemainderUnsigned{});
                break;
            case I32And::kOpcode:
                instructions.emplace_back(I32And{});
                break;
            case I32Or::kOpcode:
                instructions.emplace_back(I32Or{});
                break;
            case I32ExclusiveOr::kOpcode:
                instructions.emplace_back(I32ExclusiveOr{});
                break;
            case I32ShiftLeft::kOpcode:
                instructions.emplace_back(I32ShiftLeft{});
                break;
            case I32ShiftRightSigned::kOpcode:
                instructions.emplace_back(I32ShiftRightSigned{});
                break;
            case I32ShiftRightUnsigned::kOpcode:
                instructions.emplace_back(I32ShiftRightUnsigned{});
                break;
            case I32RotateLeft::kOpcode:
                instructions.emplace_back(I32RotateLeft{});
                break;
            case I32RotateRight::kOpcode:
                instructions.emplace_back(I32RotateRight{});
                break;
            case I32WrapI64::kOpcode:
                instructions.emplace_back(I32WrapI64{});
                break;
            case I32TruncateF32Signed::kOpcode:
                instructions.emplace_back(I32TruncateF32Signed{});
                break;
            case I32TruncateF32Unsigned::kOpcode:
                instructions.emplace_back(I32TruncateF32Unsigned{});
                break;
            case I32TruncateF64Signed::kOpcode:
                instructions.emplace_back(I32TruncateF64Signed{});
                break;
            case I32TruncateF64Unsigned::kOpcode:
                instructions.emplace_back(I32TruncateF64Unsigned{});
                break;
            case I32ReinterpretF32::kOpcode:
                instructions.emplace_back(I32ReinterpretF32{});
                break;
            case I32Extend8Signed::kOpcode:
                instructions.emplace_back(I32Extend8Signed{});
                break;
            case I32Extend16Signed::kOpcode:
                instructions.emplace_back(I32Extend16Signed{});
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
            case GlobalGet::kOpcode: {
                auto value = wasm::Leb128<std::uint32_t>::decode_from(is);
                if (!value) {
                    return std::nullopt;
                }
                instructions.emplace_back(GlobalGet{*value});
                break;
            }
            case GlobalSet::kOpcode: {
                auto value = wasm::Leb128<std::uint32_t>::decode_from(is);
                if (!value) {
                    return std::nullopt;
                }
                instructions.emplace_back(GlobalSet{*value});
                break;
            }
            case I32Load::kOpcode: {
                auto arg = parse<MemArg>(is);
                if (!arg) {
                    return std::nullopt;
                }

                instructions.emplace_back(I32Load{*arg});
                break;
            }
            case I32Store::kOpcode: {
                auto arg = parse<MemArg>(is);
                if (!arg) {
                    return std::nullopt;
                }

                instructions.emplace_back(I32Store{*arg});
                break;
            }
            default:
                std::cerr << "Unhandled opcode 0x" << std::setw(2) << std::setfill('0') << std::hex << +opcode << '\n';
                return std::nullopt;
        }
    }
}

} // namespace

tl::expected<Module, ModuleParseError> ByteCodeParser::parse_module(std::istream &is) {
    // https://webassembly.github.io/spec/core/binary/modules.html#sections
    enum class SectionId : std::uint8_t {
        Custom = 0,
        Type = 1,
        Import = 2,
        Function = 3,
        Table = 4,
        Memory = 5,
        Global = 6,
        Export = 7,
        Start = 8,
        Element = 9,
        Code = 10,
        Data = 11,
        DataCount = 12,
    };

    std::string buf;

    // https://webassembly.github.io/spec/core/binary/modules.html#binary-magic
    buf.resize(kMagicSize);
    is.read(buf.data(), buf.size());
    if (!is || buf != "\0asm"sv) {
        return tl::unexpected{ModuleParseError::InvalidMagic};
    }

    // https://webassembly.github.io/spec/core/binary/modules.html#binary-version
    buf.resize(kVersionSize);
    is.read(buf.data(), buf.size());
    if (!is || buf != "\1\0\0\0"sv) {
        return tl::unexpected{ModuleParseError::UnsupportedVersion};
    }

    Module module;

    // https://webassembly.github.io/spec/core/binary/modules.html#sections
    while (true) {
        std::uint8_t id_byte{};
        is.read(reinterpret_cast<char *>(&id_byte), sizeof(id_byte));
        if (!is) {
            // We've read 0 or more complete modules, so we're done.
            break;
        }

        if (id_byte < static_cast<int>(SectionId::Custom) || id_byte > static_cast<int>(SectionId::DataCount)) {
            return tl::unexpected{ModuleParseError::InvalidSectionId};
        }

        auto size = Leb128<std::uint32_t>::decode_from(is);
        if (!size) {
            if (size.error() == Leb128ParseError::UnexpectedEof) {
                return tl::unexpected{ModuleParseError::UnexpectedEof};
            }
            return tl::unexpected{ModuleParseError::InvalidSize};
        }

        auto id = static_cast<SectionId>(id_byte);
        switch (id) {
            case SectionId::Custom: {
                auto before = static_cast<std::int64_t>(is.tellg());
                auto name = parse<std::string>(is);
                if (!name) {
                    return tl::unexpected{ModuleParseError::InvalidCustomSection};
                }

                auto consumed_by_name = static_cast<int64_t>(is.tellg()) - before;
                auto remaining_size = static_cast<int64_t>(*size) - consumed_by_name;
                if (remaining_size < 0 || remaining_size > std::int64_t{kMaxSequenceSize}) {
                    return tl::unexpected{ModuleParseError::InvalidCustomSection};
                }

                std::vector<std::uint8_t> data;
                data.resize(remaining_size);
                if (!is.read(reinterpret_cast<char *>(data.data()), data.size())) {
                    return tl::unexpected{ModuleParseError::InvalidCustomSection};
                }

                module.custom_sections.push_back(CustomSection{
                        .name = *std::move(name),
                        .data = std::move(data),
                });
                break;
            }
            case SectionId::Type:
                module.type_section = parse_type_section(is);
                if (!module.type_section) {
                    return tl::unexpected{ModuleParseError::InvalidTypeSection};
                }
                break;
            case SectionId::Import:
                module.import_section = parse_import_section(is);
                if (!module.import_section) {
                    return tl::unexpected{ModuleParseError::InvalidImportSection};
                }
                break;
            case SectionId::Function:
                module.function_section = parse_function_section(is);
                if (!module.function_section) {
                    return tl::unexpected{ModuleParseError::InvalidFunctionSection};
                }
                break;
            case SectionId::Table:
                module.table_section = parse_table_section(is);
                if (!module.table_section) {
                    return tl::unexpected{ModuleParseError::InvalidTableSection};
                }
                break;
            case SectionId::Memory:
                module.memory_section = parse_memory_section(is);
                if (!module.memory_section) {
                    return tl::unexpected{ModuleParseError::InvalidMemorySection};
                }
                break;
            case SectionId::Global:
                module.global_section = parse_global_section(is);
                if (!module.global_section) {
                    return tl::unexpected{ModuleParseError::InvalidGlobalSection};
                }
                break;
            case SectionId::Export:
                module.export_section = parse_export_section(is);
                if (!module.export_section) {
                    return tl::unexpected{ModuleParseError::InvalidExportSection};
                }
                break;
            case SectionId::Start:
                module.start_section = parse_start_section(is);
                if (!module.start_section) {
                    return tl::unexpected{ModuleParseError::InvalidStartSection};
                }
                break;
            case SectionId::Code:
                module.code_section = parse_code_section(is);
                if (!module.code_section) {
                    return tl::unexpected{ModuleParseError::InvalidCodeSection};
                }
                break;
            case SectionId::Data:
                module.data_section = parse_data_section(is);
                if (!module.data_section) {
                    return tl::unexpected{ModuleParseError::InvalidDataSection};
                }
                break;
            case SectionId::DataCount: {
                auto count = Leb128<std::uint32_t>::decode_from(is);
                if (!count) {
                    return tl::unexpected{ModuleParseError::InvalidDataCountSection};
                }

                module.data_count_section = DataCountSection{
                        .count = *count,
                };
                break;
            }
            default:
                std::cerr << "Unhandled section: " << static_cast<int>(id) << '\n';
                // Uncomment if you want to skip past unhandled sections for e.g. debugging.
                // is.seekg(*size, std::ios::cur);
                // break;
                return tl::unexpected{ModuleParseError::UnhandledSection};
        }
    }

    return module;
}

} // namespace wasm
