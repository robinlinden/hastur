// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TYPE_TYPE_H_
#define TYPE_TYPE_H_

#include <memory>
#include <optional>
#include <string_view>

namespace type {

struct Size {
    int width{};
    int height{};
    [[nodiscard]] bool operator==(Size const &) const = default;
};

struct Px {
    int v{};
    [[nodiscard]] bool operator==(Px const &) const = default;
};

class IFont {
public:
    virtual ~IFont() = default;
    [[nodiscard]] virtual Size measure(std::string_view) const = 0;
};

class IType {
public:
    virtual ~IType() = default;
    [[nodiscard]] virtual std::optional<std::shared_ptr<IFont const>> font(std::string_view name, Px size) const = 0;
};

} // namespace type

#endif
