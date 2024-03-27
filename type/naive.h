// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TYPE_NAIVE_H_
#define TYPE_NAIVE_H_

#include "type/type.h"

#include <memory>
#include <optional>
#include <string_view>

namespace type {

class NaiveFont : public IFont {
public:
    Size measure(std::string_view text, Px font_size, Weight) const override {
        return Size{static_cast<int>(text.size()) * font_size.v / 2, font_size.v};
    }
};

class NaiveType : public IType {
public:
    std::optional<std::shared_ptr<IFont const>> font(std::string_view) const override { return font_; }

private:
    std::shared_ptr<NaiveFont> font_{std::make_shared<NaiveFont>()};
};

} // namespace type

#endif
