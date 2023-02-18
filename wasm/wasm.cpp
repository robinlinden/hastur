// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/wasm.h"

#include "wasm/leb128.h"

#include <algorithm>
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
std::optional<T> parse(std::istream &) = delete;

// https://webassembly.github.io/spec/core/bikeshed/#export-section
template<>
std::optional<Export> parse(std::istream &is) {
    // https://webassembly.github.io/spec/core/bikeshed/#binary-utf8
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

// https://webassembly.github.io/spec/core/bikeshed/#binary-vec
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

std::optional<Module> Module::parse_from(std::istream &is) {
    std::string buf;

    // https://webassembly.github.io/spec/core/bikeshed/#binary-magic
    buf.resize(kMagicSize);
    is.read(buf.data(), buf.size());
    if (!is || buf != "\0asm"sv) {
        return std::nullopt;
    }

    // https://webassembly.github.io/spec/core/bikeshed/#binary-version
    buf.resize(kVersionSize);
    is.read(buf.data(), buf.size());
    if (!is || buf != "\1\0\0\0"sv) {
        return std::nullopt;
    }

    Module module;

    // https://webassembly.github.io/spec/core/bikeshed/#sections
    while (true) {
        std::uint8_t id{};
        is.read(reinterpret_cast<char *>(&id), sizeof(id));
        if (!is) {
            // We've read 0 or more complete modules, so we're done.
            break;
        }

        if (!(id >= static_cast<int>(SectionId::Custom) && id <= static_cast<int>(SectionId::DataCount))) {
            return std::nullopt;
        }

        auto size = Leb128<std::uint32_t>::decode_from(is);
        if (!size) {
            return std::nullopt;
        }

        std::vector<std::uint8_t> content;
        content.resize(*size);
        is.read(reinterpret_cast<char *>(content.data()), *size);
        if (!is) {
            return std::nullopt;
        }

        module.sections.push_back(Section{static_cast<SectionId>(id), std::move(content)});
    }

    return module;
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

} // namespace wasm
