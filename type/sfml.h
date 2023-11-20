// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TYPE_SFML_H_
#define TYPE_SFML_H_

#include "type/type.h"

#include <SFML/Graphics/Font.hpp>

#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

namespace type {

class SfmlFont : public IFont {
public:
    SfmlFont(sf::Font const &font, Px font_size) : font_{font}, font_size_{font_size} {}

    Size measure(std::string_view text) const override;
    void set_font_size(Px size) { font_size_ = size; }

private:
    sf::Font font_{};
    Px font_size_{};
};

class SfmlType : public IType {
public:
    std::optional<std::shared_ptr<IFont const>> font(std::string_view name, Px size) const override;

private:
    mutable std::map<std::string, std::shared_ptr<SfmlFont>, std::less<>> font_cache_;
};

} // namespace type

#endif
