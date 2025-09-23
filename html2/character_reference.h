// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML2_CHARACTER_REFERENCE_H_
#define HTML2_CHARACTER_REFERENCE_H_

#include <cstdint>
#include <optional>
#include <string_view>

namespace html2 {

struct CharacterReference {
    std::string_view name;
    std::uint32_t first_codepoint{};
    std::optional<std::uint32_t> second_codepoint;
};

std::optional<CharacterReference> find_named_character_reference_for(std::string_view);

} // namespace html2

#endif
