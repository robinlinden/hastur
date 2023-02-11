// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/wasm.h"

#include "wasm/leb128.h"

#include <cstdint>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace wasm {
namespace {
constexpr int kMagicSize = 4;
constexpr int kVersionSize = 4;
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

        auto size = Uleb128::decode_from(is);
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

} // namespace wasm
