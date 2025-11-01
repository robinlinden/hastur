// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/tokenizer.h"

#include "html/token.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <variant>

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size);

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size) {
    html::Tokenizer{std::string_view{reinterpret_cast<char const *>(data), size},
            [](html::Tokenizer &tokenizer, html::Token &&token) {
                if (auto const *start_tag = std::get_if<html::StartTagToken>(&token)) {
                    if (start_tag->tag_name == "script") {
                        tokenizer.set_state(html::State::ScriptData);
                    }
                }
            }}
            .run();
    return 0;
}
