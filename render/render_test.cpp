// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "render/render.h"

#include "css/property_id.h"
#include "dom/dom.h"
#include "etest/etest.h"
#include "geom/geom.h"
#include "gfx/canvas_command_saver.h"
#include "gfx/color.h"
#include "gfx/font.h"
#include "gfx/icanvas.h"
#include "layout/layout_box.h"
#include "style/styled_node.h"

#include <string_view>
#include <utility>
#include <vector>

using etest::expect_eq;

using CanvasCommands = std::vector<gfx::CanvasCommand>;

using namespace std::literals;

constexpr auto kInvalidColor = gfx::Color{0xFF, 0, 0};

int main() {
    etest::test("text, font-family provided", [] {
        dom::Node dom = dom::Element{"span", {}, {dom::Text{"hello"}}};

        auto const &children = std::get<dom::Element>(dom).children;
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "inline"}},
                .children = {{children[0],
                        {{css::PropertyId::Display, "inline"},
                                {css::PropertyId::FontFamily, "comic sans"},
                                {css::PropertyId::FontSize, "10px"},
                                {css::PropertyId::FontStyle, "italic"}}}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Inline,
                .children = {{&styled.children[0], layout::LayoutType::Inline, {}, {}, "hello"sv}},
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

        expect_eq(saver.take_commands(),
                CanvasCommands{gfx::ClearCmd{}, gfx::FillRectCmd{expected_rect, expected_color}});
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
        auto layout = layout::LayoutBox{.node = &styled, .layout_text = "hello"sv};

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

        styled.properties.push_back({css::PropertyId::FontWeight, "bold"});

        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(),
                CanvasCommands{gfx::DrawTextWithFontOptionsCmd{
                        {0, 0},
                        "hello",
                        {"arial"},
                        16,
                        gfx::FontStyle::Bold | gfx::FontStyle::Italic,
                        gfx::Color::from_css_name("canvastext").value(),
                }});
    });

    etest::test("culling", [] {
        dom::Node dom = dom::Element{"dummy"};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "block"}, {css::PropertyId::BackgroundColor, "#010203"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Block,
                .dimensions = {{0, 0, 20, 40}},
        };

        gfx::CanvasCommandSaver saver;

        gfx::Color color{1, 2, 3};
        CanvasCommands expected{gfx::DrawRectCmd{{0, 0, 20, 40}, color}};
        // No cull rect.
        render::render_layout(saver, layout);
        expect_eq(saver.take_commands(), expected);

        // Intersecting cull rects.
        render::render_layout(saver, layout, geom::Rect{0, 0, 20, 40});
        expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{10, 10, 5, 5});
        expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{-1, -1, 100, 100});
        expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{0, 0, 1, 1});
        expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{19, 39, 1, 1});
        expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{19, 0, 1, 1});
        expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{0, 39, 1, 1});
        expect_eq(saver.take_commands(), expected);

        // Non-intersecting cull rects.
        render::render_layout(saver, layout, geom::Rect{0, 40, 1, 1});
        expect_eq(saver.take_commands(), CanvasCommands{});
        render::render_layout(saver, layout, geom::Rect{20, 40, 1, 1});
        expect_eq(saver.take_commands(), CanvasCommands{});
        render::render_layout(saver, layout, geom::Rect{20, 0, 1, 1});
        expect_eq(saver.take_commands(), CanvasCommands{});
        render::render_layout(saver, layout, geom::Rect{-1, 0, 1, 1});
        expect_eq(saver.take_commands(), CanvasCommands{});
    });

    return etest::run_all_tests();
}
