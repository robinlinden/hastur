// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TYPE_TYPE_H_
#define TYPE_TYPE_H_

#include <cstdint>
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

enum class Weight : std::uint8_t {
    Normal,
    Bold,
};

class IFont {
public:
    virtual ~IFont() = default;
    [[nodiscard]] virtual Size measure(std::string_view, Px font_size, Weight) const = 0;
};

class IType {
public:
    virtual ~IType() = default;
    [[nodiscard]] virtual std::optional<std::shared_ptr<IFont const>> font(std::string_view name) const = 0;
};

} // namespace type

#endif
