// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TYPE_NAIVE_H_
#define TYPE_NAIVE_H_

#include "type/type.h"

#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

namespace type {

class NaiveFont : public IFont {
public:
    explicit NaiveFont(Px font_size) : font_size_{font_size} {}

    Size measure(std::string_view text) const override {
        return Size{static_cast<int>(text.size()) * font_size_.v / 2, font_size_.v};
    }

private:
    Px font_size_{};
};

class NaiveType : public IType {
public:
    std::optional<std::shared_ptr<IFont const>> font(std::string_view, Px size) const override {
        if (auto font = font_cache_.find(size.v); font != font_cache_.end()) {
            return font->second;
        }

        return font_cache_.insert(std::pair{size.v, std::make_shared<NaiveFont>(size)}).first->second;
    }

private:
    mutable std::map<int, std::shared_ptr<NaiveFont>> font_cache_;
};

} // namespace type

#endif
