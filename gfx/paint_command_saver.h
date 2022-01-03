// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_PAINT_COMMAND_SAVER_H_
#define GFX_PAINT_COMMAND_SAVER_H_

#include "gfx/ipainter.h"

#include <utility>
#include <variant>
#include <vector>

namespace gfx {

struct SetViewportSizeCmd {
    int width{};
    int height{};

    [[nodiscard]] constexpr bool operator==(SetViewportSizeCmd const &) const = default;
};

struct SetScaleCmd {
    int scale{};

    [[nodiscard]] constexpr bool operator==(SetScaleCmd const &) const = default;
};

struct AddTranslationCmd {
    int dx{};
    int dy{};

    [[nodiscard]] constexpr bool operator==(AddTranslationCmd const &) const = default;
};

struct FillRectCmd {
    geom::Rect rect{};
    Color color{};

    [[nodiscard]] constexpr bool operator==(FillRectCmd const &) const = default;
};

using PaintCommand = std::variant<SetViewportSizeCmd, SetScaleCmd, AddTranslationCmd, FillRectCmd>;

class PaintCommandSaver : public IPainter {
public:
    // IPainter
    void set_viewport_size(int width, int height) override { cmds_.emplace_back(SetViewportSizeCmd{width, height}); }
    void set_scale(int scale) override { cmds_.emplace_back(SetScaleCmd{scale}); }
    void add_translation(int dx, int dy) override { cmds_.emplace_back(AddTranslationCmd{dx, dy}); }
    void fill_rect(geom::Rect const &rect, Color color) override { cmds_.emplace_back(FillRectCmd{rect, color}); }

    //
    [[nodiscard]] std::vector<PaintCommand> take_commands() { return std::exchange(cmds_, {}); }

private:
    std::vector<PaintCommand> cmds_{};
};

class PaintCommandVisitor {
public:
    constexpr PaintCommandVisitor(IPainter &painter) : painter_{painter} {}

    constexpr void operator()(SetViewportSizeCmd const &cmd) { painter_.set_viewport_size(cmd.width, cmd.height); }
    constexpr void operator()(SetScaleCmd const &cmd) { painter_.set_scale(cmd.scale); }
    constexpr void operator()(AddTranslationCmd const &cmd) { painter_.add_translation(cmd.dx, cmd.dy); }
    constexpr void operator()(FillRectCmd const &cmd) { painter_.fill_rect(cmd.rect, cmd.color); }

private:
    IPainter &painter_;
};

inline void replay_commands(IPainter &painter, std::vector<PaintCommand> const &commands) {
    PaintCommandVisitor visitor{painter};
    for (auto const &command : commands) {
        std::visit(visitor, command);
    }
}

} // namespace gfx

#endif
