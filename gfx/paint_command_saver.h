// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_PAINT_COMMAND_SAVER_H_
#define GFX_PAINT_COMMAND_SAVER_H_

#include "gfx/ipainter.h"

#include <string>
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

struct DrawTextCmd {
    geom::Position position{};
    std::string text{};
    std::string font{};
    int size{};
    Color color{};

    [[nodiscard]] bool operator==(DrawTextCmd const &) const = default;
};

using PaintCommand = std::variant<SetViewportSizeCmd, SetScaleCmd, AddTranslationCmd, FillRectCmd, DrawTextCmd>;

class PaintCommandSaver : public IPainter {
public:
    // IPainter
    void set_viewport_size(int width, int height) override { cmds_.emplace_back(SetViewportSizeCmd{width, height}); }
    void set_scale(int scale) override { cmds_.emplace_back(SetScaleCmd{scale}); }
    void add_translation(int dx, int dy) override { cmds_.emplace_back(AddTranslationCmd{dx, dy}); }
    void fill_rect(geom::Rect const &rect, Color color) override { cmds_.emplace_back(FillRectCmd{rect, color}); }
    void draw_text(geom::Position position, std::string_view text, Font font, FontSize size, Color color) override {
        cmds_.emplace_back(DrawTextCmd{position, std::string{text}, std::string{font.font}, size.px, color});
    }

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

    void operator()(DrawTextCmd const &cmd) {
        painter_.draw_text(cmd.position, cmd.text, {cmd.font}, {cmd.size}, cmd.color);
    }

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
