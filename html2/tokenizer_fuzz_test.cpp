// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/tokenizer.h"

#include "html2/token.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <variant>

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size);

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size) {
    html2::Tokenizer{std::string_view{reinterpret_cast<char const *>(data), size},
            [](html2::Tokenizer &tokenizer, html2::Token &&token) {
                if (auto const *start_tag = std::get_if<html2::StartTagToken>(&token)) {
                    if (start_tag->tag_name == "script") {
                        tokenizer.set_state(html2::State::ScriptData);
                    }
                }
            }}
            .run();
    return 0;
}
