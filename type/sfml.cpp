// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "type/sfml.h"

#include "type/type.h"

#include "os/xdg.h"
#include "util/string.h"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace type {
namespace {

std::filesystem::recursive_directory_iterator get_font_dir_iterator(std::filesystem::path const &path) {
    std::error_code errc;
    if (auto it = std::filesystem::recursive_directory_iterator(path, errc); !errc) {
        return it;
    }

    return {};
}

// TODO(robinlinden): We should be looking at font names rather than filenames.
std::optional<std::string> find_path_to_font(std::string_view font_filename) {
    for (auto const &path : os::font_paths()) {
        for (auto const &entry : get_font_dir_iterator(path)) {
            auto name = entry.path().filename().string();
            // TODO(robinlinden): std::ranges once Clang supports it. Last tested w/ 15.
            if (std::search(begin(name), end(name), begin(font_filename), end(font_filename), [](char a, char b) {
                    return util::lowercased(a) == util::lowercased(b);
                }) != end(name)) {
                return std::make_optional(entry.path().string());
            }
        }
    }

    return std::nullopt;
}

} // namespace

Size SfmlFont::measure(std::string_view text, Px font_size) const {
    sf::Text sf_text{
            sf::String::fromUtf8(text.data(), text.data() + text.size()), font_, static_cast<unsigned>(font_size.v)};
    auto bounds = sf_text.getLocalBounds();
    return Size{static_cast<int>(bounds.width), static_cast<int>(bounds.height)};
}

std::optional<std::shared_ptr<IFont const>> SfmlType::font(std::string_view name) const {
    if (auto font = font_cache_.find(name); font != font_cache_.end()) {
        return font->second;
    }

    sf::Font font;
    if (auto path = find_path_to_font(name); !path || !font.loadFromFile(*path)) {
        return std::nullopt;
    }

    return font_cache_.insert(std::pair{std::string{name}, std::make_shared<SfmlFont>(font)}).first->second;
}

} // namespace type
