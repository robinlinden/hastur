// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "render/render.h"

#include "dom/dom.h"
#include "etest/etest.h"
#include "gfx/canvas_command_saver.h"
#include "gfx/color.h"
#include "gfx/icanvas.h"
#include "layout/layout.h"
#include "style/styled_node.h"

using etest::expect_eq;

using CanvasCommands = std::vector<gfx::CanvasCommand>;

constexpr auto kInvalidColor = gfx::Color{0xFF, 0, 0};

int main() {
    etest::test("render simple layout", [] {
        dom::Node dom = dom::Element{"span", {}, {dom::Text{"hello"}}};

        auto const &children = std::get<dom::Element>(dom).children;
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "inline"}},
                .children = {{children[0], {{css::PropertyId::Display, "inline"}}, {}}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Inline,
                .dimensions = {},
                .children = {{&styled.children[0], layout::LayoutType::Inline, {}, {}, "hello"}},
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        expect_eq(
                saver.take_commands(), CanvasCommands{gfx::DrawTextWithFontOptionsCmd{{0, 0}, "hello", {"arial"}, 10}});
    });

    etest::test("text, font-family provided", [] {
        dom::Node dom = dom::Element{"span", {}, {dom::Text{"hello"}}};

        auto const &children = std::get<dom::Element>(dom).children;
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "inline"}},
                .children = {{children[0],
                        {{css::PropertyId::Display, "inline"},
                                {css::PropertyId::FontFamily, "comic sans"},
                                {css::PropertyId::FontStyle, "italic"}}}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Inline,
                .children = {{&styled.children[0], layout::LayoutType::Inline, {}, {}, "hello"}},
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        expect_eq(saver.take_commands(),
                CanvasCommands{
                        gfx::DrawTextWithFontOptionsCmd{{0, 0}, "hello", {"comic sans"}, 10, gfx::FontStyle::Italic}});
    });

    etest::test("render block with background-color", [] {
        dom::Node dom = dom::Element{"div", {}, {dom::Element{"first"}}};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "block"}, {css::PropertyId::BackgroundColor, "#0A0B0C"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Block,
                .dimensions = {{10, 20, 100, 100}, {}, {}, {}},
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        geom::Rect expected_rect{10, 20, 100, 100};
        gfx::Color expected_color{0xA, 0xB, 0xC};

        expect_eq(saver.take_commands(), CanvasCommands{gfx::DrawRectCmd{expected_rect, expected_color, {}}});
    });

    etest::test("debug-render block", [] {
        dom::Node dom = dom::Element{"div"};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "block"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Block,
                .dimensions = {{10, 20, 100, 100}, {}, {}, {}},
        };

        gfx::CanvasCommandSaver saver;
        render::debug::render_layout_depth(saver, layout);

        geom::Rect expected_rect{10, 20, 100, 100};
        gfx::Color expected_color{0xFF, 0xFF, 0xFF, 0x30};

        expect_eq(saver.take_commands(), CanvasCommands{gfx::FillRectCmd{expected_rect, expected_color}});
    });

    etest::test("render block with transparent background-color", [] {
        dom::Node dom = dom::Element{"div", {}, {dom::Element{"first"}}};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "block"}, {css::PropertyId::BackgroundColor, "transparent"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Block,
                .dimensions = {{10, 20, 100, 100}, {}, {}, {}},
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        expect_eq(saver.take_commands(), CanvasCommands{});
    });

    etest::test("render block with borders, default color", [] {
        dom::Node dom = dom::Element{"div", {}, {dom::Element{"first"}}};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "block"}, {css::PropertyId::BackgroundColor, "#0A0B0C"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Block,
                .dimensions = {{0, 0, 20, 40}, {}, {10, 10, 10, 10}, {}},
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        geom::Rect expected_rect{0, 0, 20, 40};
        gfx::Color expected_color{0xA, 0xB, 0xC};
        gfx::Borders expected_borders{{{}, 10}, {{}, 10}, {{}, 10}, {{}, 10}};

        expect_eq(saver.take_commands(),
                CanvasCommands{gfx::DrawRectCmd{expected_rect, expected_color, expected_borders}});
    });

    etest::test("render block with borders, custom color", [] {
        dom::Node dom = dom::Element{"div", {}, {dom::Element{"first"}}};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "block"},
                        {css::PropertyId::BorderLeftColor, "#010101"},
                        {css::PropertyId::BorderRightColor, "#020202"},
                        {css::PropertyId::BorderTopColor, "#030303"},
                        {css::PropertyId::BorderBottomColor, "#040404"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Block,
                .dimensions = {{0, 0, 20, 40}, {}, {2, 4, 6, 8}, {}},
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        geom::Rect expected_rect{0, 0, 20, 40};
        gfx::Color expected_color{0, 0, 0, 0};
        gfx::Borders expected_borders{{{1, 1, 1}, 2}, {{2, 2, 2}, 4}, {{3, 3, 3}, 6}, {{4, 4, 4}, 8}};

        expect_eq(saver.take_commands(),
                CanvasCommands{gfx::DrawRectCmd{expected_rect, expected_color, expected_borders}});
    });

    etest::test("currentcolor", [] {
        dom::Node dom = dom::Element{"span", {}, {dom::Element{"span"}}};
        auto const &children = std::get<dom::Element>(dom).children;

        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Color, "#aabbcc"}},
                .children = {{children[0], {{css::PropertyId::BackgroundColor, "currentcolor"}}, {}}},
        };
        styled.children[0].parent = &styled;

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Inline,
                .dimensions = {},
                .children = {{&styled.children[0], layout::LayoutType::Inline, {{0, 0, 20, 20}}, {}}},
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        auto cmd = gfx::DrawRectCmd{
                .rect{0, 0, 20, 20},
                .color{gfx::Color{0xaa, 0xbb, 0xcc}},
        };
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});
    });

    etest::test("hex colors", [] {
        dom::Node dom = dom::Element{"div"};
        auto styled = style::StyledNode{.node = dom};
        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Block,
                .dimensions = {{0, 0, 20, 20}},
        };

        gfx::CanvasCommandSaver saver;

        // #rgba
        styled.properties = {{css::PropertyId::BackgroundColor, "#abcd"}};
        auto cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{0xaa, 0xbb, 0xcc, 0xdd}}};
        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // #rrggbbaa
        styled.properties = {{css::PropertyId::BackgroundColor, "#12345678"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{0x12, 0x34, 0x56, 0x78}}};
        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // #rgb
        styled.properties = {{css::PropertyId::BackgroundColor, "#abc"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{0xaa, 0xbb, 0xcc}}};
        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // #rrggbb
        styled.properties = {{css::PropertyId::BackgroundColor, "#123456"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{0x12, 0x34, 0x56}}};
        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});
    });

    etest::test("rgba colors", [] {
        dom::Node dom = dom::Element{"div"};
        auto styled = style::StyledNode{.node = dom};
        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Block,
                .dimensions = {{0, 0, 20, 20}},
        };

        gfx::CanvasCommandSaver saver;

        // rgb, working
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2, 3)"}};
        auto cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{1, 2, 3}}};
        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // rgb, rgba should be an alias of rgb
        styled.properties = {{css::PropertyId::BackgroundColor, "rgba(100, 200, 255)"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{100, 200, 255}}};
        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // rgb, with alpha
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2, 3, 0.5)"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{1, 2, 3, 127}}};
        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // rgb, with alpha
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2, 3, 0.2)"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{1, 2, 3, 51}}};
        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // rgb, alpha out of range
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2, 3, 2)"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{1, 2, 3, 0xFF}}};
        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // rgb, garbage values in alpha
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2, 3, blergh)"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{kInvalidColor}};
        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // rgb, missing closing paren
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2, 3"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{kInvalidColor}};
        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // rgb, value out of range
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(-1, 2, 3)"}};
        render::render_layout(saver, layout);
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{kInvalidColor}};
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // rgb, wrong number of arguments
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2)"}};
        render::render_layout(saver, layout);
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{kInvalidColor}};
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // rgb, garbage value
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(a, 2, 3)"}};
        render::render_layout(saver, layout);
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{kInvalidColor}};
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});
    });

    etest::test("render block with borders, custom color", [] {
        dom::Node dom = dom::Text{"hello"};
        auto styled = style::StyledNode{.node = dom,
                .properties = {
                        {css::PropertyId::TextDecorationLine, "line-through"},
                        {css::PropertyId::FontFamily, "arial"},
                        {css::PropertyId::FontSize, "16px"},
                }};
        auto layout = layout::LayoutBox{.node = &styled, .layout_text = "hello"};

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        expect_eq(saver.take_commands(),
                CanvasCommands{gfx::DrawTextWithFontOptionsCmd{
                        {0, 0},
                        "hello",
                        {"arial"},
                        16,
                        gfx::FontStyle::Strikethrough,
                        gfx::Color::from_css_name("canvastext").value(),
                }});

        styled.properties[0].second = "underline";
        styled.properties.push_back({css::PropertyId::FontStyle, "italic"});

        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(),
                CanvasCommands{gfx::DrawTextWithFontOptionsCmd{
                        {0, 0},
                        "hello",
                        {"arial"},
                        16,
                        gfx::FontStyle::Underlined | gfx::FontStyle::Italic,
                        gfx::Color::from_css_name("canvastext").value(),
                }});

        styled.properties[0].second = "blink";

        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(),
                CanvasCommands{gfx::DrawTextWithFontOptionsCmd{
                        {0, 0},
                        "hello",
                        {"arial"},
                        16,
                        gfx::FontStyle::Italic,
                        gfx::Color::from_css_name("canvastext").value(),
                }});
    });

    return etest::run_all_tests();
}
