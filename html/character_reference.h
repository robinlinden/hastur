// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_CHARACTER_REFERENCE_H_
#define HTML_CHARACTER_REFERENCE_H_

#include <cstdint>
#include <optional>
#include <string_view>

namespace html {

struct CharacterReference {
    std::string_view name;
    std::uint32_t first_codepoint{};
    std::optional<std::uint32_t> second_codepoint;
};

std::optional<CharacterReference> find_named_character_reference_for(std::string_view);

} // namespace html

#endif
