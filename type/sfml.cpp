// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "type/sfml.h"

#include "type/fallback_font.h"
#include "type/type.h"

#include "os/xdg.h"
#include "util/string.h"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

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
std::vector<std::string> find_path_to_font(std::string_view font_filename) {
    std::vector<std::string> paths;
    for (auto const &path : os::font_paths()) {
        for (auto const &entry : get_font_dir_iterator(path)) {
            auto name = entry.path().filename().string();
            if (!std::ranges::search(name, font_filename, [](char a, char b) {
                    return util::lowercased(a) == util::lowercased(b);
                }).empty()) {
                paths.push_back(entry.path().string());
            }
        }
    }

    return paths;
}

} // namespace

Size SfmlFont::measure(std::string_view text, Px font_size, Weight weight) const {
    sf::Text sf_text{
            font_, sf::String::fromUtf8(text.data(), text.data() + text.size()), static_cast<unsigned>(font_size.v)};

    switch (weight) {
        case Weight::Normal:
            sf_text.setStyle(sf::Text::Regular);
            break;
        case Weight::Bold:
            sf_text.setStyle(sf::Text::Bold);
            break;
    }

    // Workaround for SFML saying that non-breaking spaces have 0 width.
    int nbsp_extra_width = 0;
    auto const nbsp_count = static_cast<int>(std::ranges::count(sf_text.getString(), std::uint32_t{0xA0}));
    if (nbsp_count > 0) {
        sf::Text sf_space{font_, sf::String{" "}, static_cast<unsigned>(font_size.v)};
        if (weight == Weight::Bold) {
            sf_space.setStyle(sf::Text::Bold);
        }

        nbsp_extra_width = static_cast<int>(sf_space.getLocalBounds().size.x);
        nbsp_extra_width *= nbsp_count;
    }

    auto bounds = sf_text.getLocalBounds();
    return Size{static_cast<int>(bounds.size.x) + nbsp_extra_width, static_cast<int>(bounds.size.y)};
}

std::optional<std::shared_ptr<IFont const>> SfmlType::font(std::string_view name) const {
    if (auto font = font_cache_.find(name); font != font_cache_.end()) {
        return font->second;
    }

    auto candidates = find_path_to_font(name);
    for (auto const &path : candidates) {
        sf::Font font;
        if (!font.openFromFile(path)) {
            spdlog::warn("Failed to load font '{}'", path);
            continue;
        }

        if (!font.hasGlyph('A')) {
            spdlog::warn("Font '{}' ({}) does not have an 'A' glyph", font.getInfo().family, name);
            font_cache_.insert(std::pair{std::string{name}, std::nullopt});
            continue;
        }

        spdlog::info("Loaded font '{}' as '{}", path, name);
        return font_cache_.insert(std::pair{std::string{name}, std::make_shared<SfmlFont>(font)}).first->second;
    }

    font_cache_.insert(std::pair{std::string{name}, std::nullopt});
    return std::nullopt;
}

std::shared_ptr<SfmlFont const> SfmlType::fallback_font() const {
    if (fallback_font_ == nullptr) {
        auto font_data = type::fallback_font_ttf_data();
        sf::Font font;
        [[maybe_unused]] auto res = font.openFromMemory(font_data.data(), font_data.size());
        assert(res);
        fallback_font_ = std::make_shared<SfmlFont>(font);
    }

    return fallback_font_;
}

} // namespace type
