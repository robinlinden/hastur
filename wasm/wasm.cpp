// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/wasm.h"

#include "wasm/leb128.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
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

constexpr int kMagicSize = 4;
constexpr int kVersionSize = 4;

template<typename T>
std::optional<std::vector<T>> parse_vector(std::istream &);
template<typename T>
std::optional<std::vector<T>> parse_vector(std::istream &&);

template<typename T>
std::optional<T> parse(std::istream &) = delete;

template<>
std::optional<std::uint32_t> parse(std::istream &is) {
    return Leb128<std::uint32_t>::decode_from(is);
}

template<>
std::optional<ValueType> parse(std::istream &is) {
    return ValueType::parse(is);
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

// https://webassembly.github.io/spec/core/binary/modules.html#binary-exportsec
template<>
std::optional<Export> parse(std::istream &is) {
    // https://webassembly.github.io/spec/core/binary/values.html#names
    auto name_length = Leb128<std::uint32_t>::decode_from(is);
    if (!name_length) {
        return std::nullopt;
    }
    std::string name;
    name.reserve(*name_length);
    for (std::uint32_t i = 0; i < *name_length; ++i) {
        // TODO(robinlinden): Handle non-ascii.
        char c{};
        if (!is.read(reinterpret_cast<char *>(&c), sizeof(c)) || c > 0x79) {
            return std::nullopt;
        }

        name += c;
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
            .name = std::move(name),
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

    auto type = ValueType::parse(is);
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

    auto cursor_before_locals = is.tellg();

    auto locals = parse_vector<CodeEntry::Local>(is);
    if (!locals) {
        return std::nullopt;
    }

    auto bytes_consumed_by_locals = is.tellg() - cursor_before_locals;
    assert(bytes_consumed_by_locals >= 0);

    std::vector<std::uint8_t> code;
    code.resize(*size - bytes_consumed_by_locals);
    if (!is.read(reinterpret_cast<char *>(code.data()), code.size())) {
        return std::nullopt;
    }

    return CodeEntry{
            .code = std::move(code),
            .locals = *std::move(locals),
    };
}

// https://webassembly.github.io/spec/core/binary/conventions.html#vectors
template<typename T>
std::optional<std::vector<T>> parse_vector(std::istream &is) {
    auto item_count = Leb128<std::uint32_t>::decode_from(is);
    if (!item_count) {
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

std::optional<std::string> get_section_data(std::vector<Section> const &sections, SectionId id) {
    auto section = std::ranges::find_if(sections, [&](auto const &s) { return s.id == id; });
    if (section == end(sections)) {
        return std::nullopt;
    }

    return std::string{reinterpret_cast<char const *>(section->content.data()), section->content.size()};
}

} // namespace

// https://webassembly.github.io/spec/core/binary/types.html
std::optional<ValueType> ValueType::parse(std::istream &is) {
    std::uint8_t byte{};
    if (!is.read(reinterpret_cast<char *>(&byte), sizeof(byte))) {
        return std::nullopt;
    }

    switch (byte) {
        case 0x7f:
            return ValueType{Kind::Int32};
        case 0x7e:
            return ValueType{Kind::Int64};
        case 0x7d:
            return ValueType{Kind::Float32};
        case 0x7c:
            return ValueType{Kind::Float64};
        case 0x7b:
            return ValueType{Kind::Vector128};
        case 0x70:
            return ValueType{Kind::FunctionReference};
        case 0x6f:
            return ValueType{Kind::ExternReference};
        default:
            return std::nullopt;
    }
}

tl::expected<Module, ParseError> Module::parse_from(std::istream &is) {
    std::string buf;

    // https://webassembly.github.io/spec/core/binary/modules.html#binary-magic
    buf.resize(kMagicSize);
    is.read(buf.data(), buf.size());
    if (!is || buf != "\0asm"sv) {
        return tl::unexpected{ParseError::InvalidMagic};
    }

    // https://webassembly.github.io/spec/core/binary/modules.html#binary-version
    buf.resize(kVersionSize);
    is.read(buf.data(), buf.size());
    if (!is || buf != "\1\0\0\0"sv) {
        return tl::unexpected{ParseError::UnsupportedVersion};
    }

    Module module;

    // https://webassembly.github.io/spec/core/binary/modules.html#sections
    while (true) {
        std::uint8_t id{};
        is.read(reinterpret_cast<char *>(&id), sizeof(id));
        if (!is) {
            // We've read 0 or more complete modules, so we're done.
            break;
        }

        if (!(id >= static_cast<int>(SectionId::Custom) && id <= static_cast<int>(SectionId::DataCount))) {
            return tl::unexpected{ParseError::InvalidSectionId};
        }

        // TODO(robinlinden): Propagate error from leb128-parsing.
        auto size = Leb128<std::uint32_t>::decode_from(is);
        if (!size) {
            return tl::unexpected{ParseError::Unknown};
        }

        std::vector<std::uint8_t> content;
        content.resize(*size);
        is.read(reinterpret_cast<char *>(content.data()), *size);
        if (!is) {
            return tl::unexpected{ParseError::UnexpectedEof};
        }

        module.sections.push_back(Section{static_cast<SectionId>(id), std::move(content)});
    }

    return module;
}

std::optional<TypeSection> Module::type_section() const {
    auto content = get_section_data(sections, SectionId::Type);
    if (!content) {
        return std::nullopt;
    }

    if (auto maybe_types = parse_vector<FunctionType>(std::stringstream{*std::move(content)})) {
        return TypeSection{.types = *std::move(maybe_types)};
    }

    return std::nullopt;
}

std::optional<FunctionSection> Module::function_section() const {
    auto content = get_section_data(sections, SectionId::Function);
    if (!content) {
        return std::nullopt;
    }

    if (auto maybe_type_indices = parse_vector<TypeIdx>(std::stringstream{*std::move(content)})) {
        return FunctionSection{.type_indices = *std::move(maybe_type_indices)};
    }

    return std::nullopt;
}

std::optional<ExportSection> Module::export_section() const {
    auto content = get_section_data(sections, SectionId::Export);
    if (!content) {
        return std::nullopt;
    }

    if (auto maybe_exports = parse_vector<Export>(std::stringstream{*std::move(content)})) {
        return ExportSection{.exports = std::move(maybe_exports).value()};
    }

    return std::nullopt;
}

std::optional<CodeSection> Module::code_section() const {
    auto content = get_section_data(sections, SectionId::Code);
    if (!content) {
        return std::nullopt;
    }

    if (auto code_entries = parse_vector<CodeEntry>(std::stringstream{*std::move(content)})) {
        return CodeSection{.entries = *std::move(code_entries)};
    }

    return std::nullopt;
}

} // namespace wasm
