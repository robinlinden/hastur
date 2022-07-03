// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_CANVAS_COMMAND_SAVER_H_
#define GFX_CANVAS_COMMAND_SAVER_H_

#include "gfx/icanvas.h"

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

struct DrawRectCmd {
    geom::Rect rect{};
    Color color{};
    Borders borders{};

    [[nodiscard]] constexpr bool operator==(DrawRectCmd const &) const = default;
};

struct DrawTextCmd {
    geom::Position position{};
    std::string text{};
    std::string font{};
    int size{};
    Color color{};

    [[nodiscard]] bool operator==(DrawTextCmd const &) const = default;
};

using CanvasCommand =
        std::variant<SetViewportSizeCmd, SetScaleCmd, AddTranslationCmd, FillRectCmd, DrawRectCmd, DrawTextCmd>;

class CanvasCommandSaver : public ICanvas {
public:
    // ICanvas
    void set_viewport_size(int width, int height) override { cmds_.emplace_back(SetViewportSizeCmd{width, height}); }
    void set_scale(int scale) override { cmds_.emplace_back(SetScaleCmd{scale}); }
    void add_translation(int dx, int dy) override { cmds_.emplace_back(AddTranslationCmd{dx, dy}); }
    void fill_rect(geom::Rect const &rect, Color color) override { cmds_.emplace_back(FillRectCmd{rect, color}); }
    void draw_rect(geom::Rect const &rect, Color const &color, Borders const &borders) override {
        cmds_.emplace_back(DrawRectCmd{rect, color, borders});
    }
    void draw_text(geom::Position position, std::string_view text, Font font, FontSize size, Color color) override {
        cmds_.emplace_back(DrawTextCmd{position, std::string{text}, std::string{font.font}, size.px, color});
    }

    //
    [[nodiscard]] std::vector<CanvasCommand> take_commands() { return std::exchange(cmds_, {}); }

private:
    std::vector<CanvasCommand> cmds_{};
};

class CanvasCommandVisitor {
public:
    constexpr CanvasCommandVisitor(ICanvas &canvas) : canvas_{canvas} {}

    constexpr void operator()(SetViewportSizeCmd const &cmd) { canvas_.set_viewport_size(cmd.width, cmd.height); }
    constexpr void operator()(SetScaleCmd const &cmd) { canvas_.set_scale(cmd.scale); }
    constexpr void operator()(AddTranslationCmd const &cmd) { canvas_.add_translation(cmd.dx, cmd.dy); }
    constexpr void operator()(FillRectCmd const &cmd) { canvas_.fill_rect(cmd.rect, cmd.color); }
    constexpr void operator()(DrawRectCmd const &cmd) { canvas_.draw_rect(cmd.rect, cmd.color, cmd.borders); }

    void operator()(DrawTextCmd const &cmd) {
        canvas_.draw_text(cmd.position, cmd.text, {cmd.font}, {cmd.size}, cmd.color);
    }

private:
    ICanvas &canvas_;
};

inline void replay_commands(ICanvas &canvas, std::vector<CanvasCommand> const &commands) {
    CanvasCommandVisitor visitor{canvas};
    for (auto const &command : commands) {
        std::visit(visitor, command);
    }
}

} // namespace gfx

#endif
