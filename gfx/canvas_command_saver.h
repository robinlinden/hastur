// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_CANVAS_COMMAND_SAVER_H_
#define GFX_CANVAS_COMMAND_SAVER_H_

#include "gfx/color.h"
#include "gfx/font.h"
#include "gfx/icanvas.h"

#include "geom/geom.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
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

struct ClearCmd {
    Color color{};

    [[nodiscard]] constexpr bool operator==(ClearCmd const &) const = default;
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
    Corners corners{};

    [[nodiscard]] constexpr bool operator==(DrawRectCmd const &) const = default;
};

struct DrawTextWithFontOptionsCmd {
    geom::Position position{};
    std::string text{};
    std::vector<std::string> font_options{};
    int size{};
    FontStyle style{FontStyle::Normal};
    Color color{};

    [[nodiscard]] bool operator==(DrawTextWithFontOptionsCmd const &) const = default;
};

struct DrawTextCmd {
    geom::Position position{};
    std::string text{};
    std::string font{};
    int size{};
    FontStyle style{FontStyle::Normal};
    Color color{};

    [[nodiscard]] bool operator==(DrawTextCmd const &) const = default;
};

struct DrawPixelsCmd {
    geom::Rect rect{};
    std::vector<std::uint8_t> rgba_data{};

    [[nodiscard]] bool operator==(DrawPixelsCmd const &) const = default;
};

using CanvasCommand = std::variant<SetViewportSizeCmd,
        SetScaleCmd,
        AddTranslationCmd,
        ClearCmd,
        FillRectCmd,
        DrawRectCmd,
        DrawTextWithFontOptionsCmd,
        DrawTextCmd,
        DrawPixelsCmd>;

class CanvasCommandSaver : public ICanvas {
public:
    // ICanvas
    void set_viewport_size(int width, int height) override { cmds_.emplace_back(SetViewportSizeCmd{width, height}); }
    void set_scale(int scale) override { cmds_.emplace_back(SetScaleCmd{scale}); }
    void add_translation(int dx, int dy) override { cmds_.emplace_back(AddTranslationCmd{dx, dy}); }
    void clear(Color c) override { cmds_.emplace_back(ClearCmd{c}); }
    void fill_rect(geom::Rect const &rect, Color color) override { cmds_.emplace_back(FillRectCmd{rect, color}); }
    void draw_rect(
            geom::Rect const &rect, Color const &color, Borders const &borders, Corners const &corners) override {
        cmds_.emplace_back(DrawRectCmd{rect, color, borders, corners});
    }
    void draw_text(geom::Position position,
            std::string_view text,
            std::span<Font const> font_options,
            FontSize size,
            FontStyle style,
            Color color) override {
        std::vector<std::string> copied_options;
        std::ranges::transform(font_options, std::back_inserter(copied_options), [](auto const &font) {
            return std::string{font.font};
        });
        cmds_.emplace_back(DrawTextWithFontOptionsCmd{
                position, std::string{text}, std::move(copied_options), size.px, style, color});
    }

    void draw_text(geom::Position position,
            std::string_view text,
            Font font,
            FontSize size,
            FontStyle style,
            Color color) override {
        cmds_.emplace_back(DrawTextCmd{position, std::string{text}, std::string{font.font}, size.px, style, color});
    }

    void draw_pixels(geom::Rect const &rect, std::span<std::uint8_t const> rgba_data) override {
        cmds_.emplace_back(DrawPixelsCmd{rect, {rgba_data.begin(), rgba_data.end()}});
    }

    //
    [[nodiscard]] std::vector<CanvasCommand> take_commands() { return std::exchange(cmds_, {}); }

private:
    std::vector<CanvasCommand> cmds_{};
};

class CanvasCommandVisitor {
public:
    constexpr explicit CanvasCommandVisitor(ICanvas &canvas) : canvas_{canvas} {}

    constexpr void operator()(SetViewportSizeCmd const &cmd) { canvas_.set_viewport_size(cmd.width, cmd.height); }
    constexpr void operator()(SetScaleCmd const &cmd) { canvas_.set_scale(cmd.scale); }
    constexpr void operator()(AddTranslationCmd const &cmd) { canvas_.add_translation(cmd.dx, cmd.dy); }
    constexpr void operator()(ClearCmd const &cmd) { canvas_.clear(cmd.color); }
    constexpr void operator()(FillRectCmd const &cmd) { canvas_.fill_rect(cmd.rect, cmd.color); }

    constexpr void operator()(DrawRectCmd const &cmd) {
        canvas_.draw_rect(cmd.rect, cmd.color, cmd.borders, cmd.corners);
    }

    constexpr void operator()(DrawTextWithFontOptionsCmd const &cmd) {
        std::vector<gfx::Font> fonts;
        std::ranges::transform(
                cmd.font_options, std::back_inserter(fonts), [](auto const &font) { return gfx::Font{font}; });
        canvas_.draw_text(cmd.position, cmd.text, fonts, {cmd.size}, cmd.style, cmd.color);
    }

    void operator()(DrawTextCmd const &cmd) {
        canvas_.draw_text(cmd.position, cmd.text, {cmd.font}, {cmd.size}, cmd.style, cmd.color);
    }

    void operator()(DrawPixelsCmd const &cmd) { canvas_.draw_pixels(cmd.rect, cmd.rgba_data); }

private:
    ICanvas &canvas_;
};

constexpr void replay_commands(ICanvas &canvas, std::vector<CanvasCommand> const &commands) {
    CanvasCommandVisitor visitor{canvas};
    for (auto const &command : commands) {
        std::visit(visitor, command);
    }
}

} // namespace gfx

#endif
