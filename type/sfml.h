// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TYPE_SFML_H_
#define TYPE_SFML_H_

#include "type/type.h"

#include <SFML/Graphics/Font.hpp>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace type {

class SfmlFont : public IFont {
public:
    explicit SfmlFont(sf::Font const &font) : font_{font} {}

    Size measure(std::string_view text, Px font_size, Weight) const override;

    sf::Font const &sf_font() const { return font_; }

private:
    sf::Font font_{};
};

class SfmlType : public IType {
public:
    std::optional<std::shared_ptr<IFont const>> font(std::string_view name) const override;

    void set_font(std::string name, std::optional<std::shared_ptr<SfmlFont const>> font) {
        font_cache_.insert_or_assign(std::move(name), std::move(font));
    }

private:
    mutable std::map<std::string, std::optional<std::shared_ptr<SfmlFont const>>, std::less<>> font_cache_;
};

} // namespace type

#endif
