// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "render/render.h"

#include "css/property_id.h"
#include "dom/dom.h"
#include "etest/etest2.h"
#include "geom/geom.h"
#include "gfx/canvas_command_saver.h"
#include "gfx/color.h"
#include "gfx/icanvas.h"
#include "layout/layout_box.h"
#include "style/styled_node.h"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

using CanvasCommands = std::vector<gfx::CanvasCommand>;

using namespace std::literals;

constexpr auto kInvalidColor = gfx::Color{0xFF, 0, 0};

int main() {
    etest::Suite s{};

    s.add_test("text, font-family provided", [](etest::IActions &a) {
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
                .children{{
                        .children{{&styled.children[0], {}, {}, "hello"sv}},
                }},
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        a.expect_eq(saver.take_commands(),
                CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}},
                        gfx::DrawTextWithFontOptionsCmd{{0, 0}, "hello", {"comic sans"}, 10, {.italic = true}}});
    });

    s.add_test("render block with background-color", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"div", {}, {dom::Element{"first"}}};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "block"}, {css::PropertyId::BackgroundColor, "#0A0B0C"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .dimensions = {{10, 20, 100, 100}, {}, {}, {}},
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        geom::Rect expected_rect{10, 20, 100, 100};
        gfx::Color expected_color{0xA, 0xB, 0xC};

        a.expect_eq(saver.take_commands(),
                CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, gfx::DrawRectCmd{expected_rect, expected_color, {}}});
    });

    s.add_test("debug-render block", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"div"};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "block"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .dimensions = {{10, 20, 100, 100}, {}, {}, {}},
                .children{
                        {nullptr, {{10, 20, 10, 10}}, {}, "hello"sv},
                        {nullptr, {{10, 30, 10, 10}}, {}, "world"sv},
                },
        };

        gfx::CanvasCommandSaver saver;
        render::debug::render_layout_depth(saver, layout);

        gfx::Color c{0xFF, 0xFF, 0xFF, 0x30};
        a.expect_eq(saver.take_commands(),
                CanvasCommands{gfx::ClearCmd{},
                        gfx::DrawRectCmd{{10, 20, 100, 100}, c},
                        gfx::DrawRectCmd{{10, 20, 10, 10}, c},
                        gfx::DrawRectCmd{{10, 30, 10, 10}, c}});
    });

    s.add_test("render block with transparent background-color", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"div", {}, {dom::Element{"first"}}};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "block"}, {css::PropertyId::BackgroundColor, "transparent"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .dimensions = {{10, 20, 100, 100}, {}, {}, {}},
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});
    });

    s.add_test("render block with borders, default color", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"div", {}, {dom::Element{"first"}}};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "block"}, {css::PropertyId::BackgroundColor, "#0A0B0C"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .dimensions = {{0, 0, 20, 40}, {}, {10, 10, 10, 10}, {}},
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        geom::Rect expected_rect{0, 0, 20, 40};
        gfx::Color expected_color{0xA, 0xB, 0xC};
        gfx::Borders expected_borders{{{}, 10}, {{}, 10}, {{}, 10}, {{}, 10}};

        a.expect_eq(saver.take_commands(),
                CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}},
                        gfx::DrawRectCmd{expected_rect, expected_color, expected_borders}});
    });

    s.add_test("render block with borders, custom color", [](etest::IActions &a) {
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
                .dimensions = {{0, 0, 20, 40}, {}, {2, 4, 6, 8}, {}},
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        geom::Rect expected_rect{0, 0, 20, 40};
        gfx::Color expected_color{0, 0, 0, 0};
        gfx::Borders expected_borders{{{1, 1, 1}, 2}, {{2, 2, 2}, 4}, {{3, 3, 3}, 6}, {{4, 4, 4}, 8}};

        a.expect_eq(saver.take_commands(),
                CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}},
                        gfx::DrawRectCmd{expected_rect, expected_color, expected_borders}});
    });

    s.add_test("render img", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"img", {{"src", "meep.png"}}};
        auto styled = style::StyledNode{.node = dom};
        auto layout = layout::LayoutBox{
                .node = &styled,
                .dimensions = {{0, 0, 1, 3}},
        };

        std::vector<std::uint8_t> img{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        auto get_img_success = [&](auto) -> render::ImageView {
            return {1, 3, img};
        };
        auto get_img_failure = [&](auto) -> std::optional<render::ImageView> {
            return std::nullopt;
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout, {}, get_img_success);

        // Success!
        a.expect_eq(saver.take_commands(),
                CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, gfx::DrawPixelsCmd{{0, 0, 1, 3}, img}});

        // Failure: blocks aren't supported yet
        styled.properties = {{css::PropertyId::Display, "block"}};
        render::render_layout(saver, layout, {}, get_img_success);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});
        styled.properties.clear();

        // Failure: image not found
        render::render_layout(saver, layout, {}, get_img_failure);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});

        // // Failure: missing src
        dom = dom::Element{"img"};
        render::render_layout(saver, layout, {}, get_img_success);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});

        // // Failure: not an img
        dom = dom::Element{"div", {{"src", "meep.png"}}};
        render::render_layout(saver, layout, {}, get_img_success);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});
    });

    s.add_test("currentcolor", [](etest::IActions &a) {
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
                .dimensions = {},
                .children = {{&styled.children[0], {{0, 0, 20, 20}}, {}}},
        };

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        auto cmd = gfx::DrawRectCmd{
                .rect{0, 0, 20, 20},
                .color{gfx::Color{0xaa, 0xbb, 0xcc}},
        };
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});
    });

    s.add_test("hex colors", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"div"};
        auto styled = style::StyledNode{.node = dom};
        auto layout = layout::LayoutBox{
                .node = &styled,
                .dimensions = {{0, 0, 20, 20}},
        };

        gfx::CanvasCommandSaver saver;

        // #rgba
        styled.properties = {{css::PropertyId::BackgroundColor, "#abcd"}};
        auto cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{0xaa, 0xbb, 0xcc, 0xdd}}};
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});

        // #rrggbbaa
        styled.properties = {{css::PropertyId::BackgroundColor, "#12345678"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{0x12, 0x34, 0x56, 0x78}}};
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});

        // #rgb
        styled.properties = {{css::PropertyId::BackgroundColor, "#abc"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{0xaa, 0xbb, 0xcc}}};
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});

        // #rrggbb
        styled.properties = {{css::PropertyId::BackgroundColor, "#123456"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{0x12, 0x34, 0x56}}};
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});
    });

    s.add_test("rgba colors", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"div"};
        auto styled = style::StyledNode{.node = dom};
        auto layout = layout::LayoutBox{
                .node = &styled,
                .dimensions = {{0, 0, 20, 20}},
        };

        gfx::CanvasCommandSaver saver;

        // rgb, working
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2, 3)"}};
        auto cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{1, 2, 3}}};
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});

        // rgb, rgba should be an alias of rgb
        styled.properties = {{css::PropertyId::BackgroundColor, "rgba(100, 200, 255)"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{100, 200, 255}}};
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});

        // rgb, with alpha
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2, 3, 0.5)"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{1, 2, 3, 127}}};
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});

        // rgb, with alpha
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2, 3, 0.2)"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{1, 2, 3, 51}}};
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});

        // rgb, alpha out of range
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2, 3, 2)"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{1, 2, 3, 0xFF}}};
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});

        // rgb, garbage values in alpha
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2, 3, blergh)"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{kInvalidColor}};
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});

        // rgb, missing closing paren
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2, 3"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{kInvalidColor}};
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});

        // rgb, value out of range
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(-1, 2, 3)"}};
        render::render_layout(saver, layout);
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{kInvalidColor}};
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});

        // rgb, wrong number of arguments
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(1, 2)"}};
        render::render_layout(saver, layout);
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{kInvalidColor}};
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});

        // rgb, garbage value
        styled.properties = {{css::PropertyId::BackgroundColor, "rgb(a, 2, 3)"}};
        render::render_layout(saver, layout);
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{kInvalidColor}};
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, cmd});
    });

    s.add_test("text style", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"dummy"};
        auto styled = style::StyledNode{.node = dom,
                .properties = {
                        {css::PropertyId::TextDecorationLine, "line-through"},
                        {css::PropertyId::FontFamily, "arial"},
                        {css::PropertyId::FontSize, "16px"},
                }};
        auto layout = layout::LayoutBox{.node = &styled, .layout_text = "hello"sv};

        gfx::CanvasCommandSaver saver;
        render::render_layout(saver, layout);

        a.expect_eq(saver.take_commands(),
                CanvasCommands{
                        gfx::ClearCmd{{0xFF, 0xFF, 0xFF}},
                        gfx::DrawTextWithFontOptionsCmd{
                                {0, 0},
                                "hello",
                                {"arial"},
                                16,
                                {.strikethrough = true},
                                gfx::Color::from_css_name("canvastext").value(),
                        },
                });

        styled.properties[0].second = "underline";
        styled.properties.emplace_back(css::PropertyId::FontStyle, "italic");

        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(),
                CanvasCommands{
                        gfx::ClearCmd{{0xFF, 0xFF, 0xFF}},
                        gfx::DrawTextWithFontOptionsCmd{
                                {0, 0},
                                "hello",
                                {"arial"},
                                16,
                                {.italic = true, .underlined = true},
                                gfx::Color::from_css_name("canvastext").value(),
                        },
                });

        styled.properties[0].second = "blink";

        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(),
                CanvasCommands{
                        gfx::ClearCmd{{0xFF, 0xFF, 0xFF}},
                        gfx::DrawTextWithFontOptionsCmd{
                                {0, 0},
                                "hello",
                                {"arial"},
                                16,
                                {.italic = true},
                                gfx::Color::from_css_name("canvastext").value(),
                        },
                });

        styled.properties[0].second = "overline";

        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(),
                CanvasCommands{
                        gfx::ClearCmd{{0xFF, 0xFF, 0xFF}},
                        gfx::DrawTextWithFontOptionsCmd{
                                {0, 0},
                                "hello",
                                {"arial"},
                                16,
                                {.italic = true, .overlined = true},
                                gfx::Color::from_css_name("canvastext").value(),
                        },
                });

        styled.properties.emplace_back(css::PropertyId::FontWeight, "bold");

        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(),
                CanvasCommands{
                        gfx::ClearCmd{{0xFF, 0xFF, 0xFF}},
                        gfx::DrawTextWithFontOptionsCmd{
                                {0, 0},
                                "hello",
                                {"arial"},
                                16,
                                {.bold = true, .italic = true, .overlined = true},
                                gfx::Color::from_css_name("canvastext").value(),
                        },
                });
    });

    s.add_test("culling", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"dummy"};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "block"}, {css::PropertyId::BackgroundColor, "#010203"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .dimensions = {{0, 0, 20, 40}},
        };

        gfx::CanvasCommandSaver saver;

        gfx::Color color{1, 2, 3};
        CanvasCommands expected{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}, gfx::DrawRectCmd{{0, 0, 20, 40}, color}};
        // No cull rect.
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), expected);

        // Intersecting cull rects.
        render::render_layout(saver, layout, geom::Rect{0, 0, 20, 40});
        a.expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{10, 10, 5, 5});
        a.expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{-1, -1, 100, 100});
        a.expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{0, 0, 1, 1});
        a.expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{19, 39, 1, 1});
        a.expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{19, 0, 1, 1});
        a.expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{0, 39, 1, 1});
        a.expect_eq(saver.take_commands(), expected);

        // Non-intersecting cull rects.
        render::render_layout(saver, layout, geom::Rect{0, 40, 1, 1});
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});
        render::render_layout(saver, layout, geom::Rect{20, 40, 1, 1});
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});
        render::render_layout(saver, layout, geom::Rect{20, 0, 1, 1});
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});
        render::render_layout(saver, layout, geom::Rect{-1, 0, 1, 1});
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});
    });

    s.add_test("culling w/ element border", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"dummy"};
        auto styled = style::StyledNode{
                .node = dom,
                .properties{
                        {css::PropertyId::Display, "block"},
                        {css::PropertyId::BackgroundColor, "#010203"},
                        {css::PropertyId::BorderLeftWidth, "1px"},
                        {css::PropertyId::BorderRightWidth, "1px"},
                        {css::PropertyId::BorderTopWidth, "1px"},
                        {css::PropertyId::BorderBottomWidth, "1px"},
                        {css::PropertyId::BorderLeftColor, "#070809"},
                        {css::PropertyId::BorderRightColor, "#0A0B0C"},
                        {css::PropertyId::BorderTopColor, "#0D0E0F"},
                        {css::PropertyId::BorderBottomColor, "#101112"},
                        {css::PropertyId::BorderLeftStyle, "solid"},
                        {css::PropertyId::BorderRightStyle, "solid"},
                        {css::PropertyId::BorderTopStyle, "solid"},
                        {css::PropertyId::BorderBottomStyle, "solid"},
                },
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .dimensions = {.content{0, 0, 20, 40}, .border{1, 1, 1, 1}},
        };

        gfx::CanvasCommandSaver saver;

        gfx::Color color{1, 2, 3};
        CanvasCommands expected{
                gfx::ClearCmd{{0xFF, 0xFF, 0xFF}},
                gfx::DrawRectCmd{
                        {0, 0, 20, 40},
                        color,
                        {
                                .left{gfx::Color{0x07, 0x08, 0x09}, 1},
                                .right{gfx::Color{0x0A, 0x0B, 0x0C}, 1},
                                .top{gfx::Color{0x0D, 0x0E, 0x0F}, 1},
                                .bottom{gfx::Color{0x10, 0x11, 0x12}, 1},
                        },
                },
        };
        // No cull rect.
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), expected);

        // Intersecting cull rects.
        render::render_layout(saver, layout, geom::Rect{-1, -1, 22, 42});
        a.expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{10, 10, 5, 5});
        a.expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{-2, -2, 100, 100});
        a.expect_eq(saver.take_commands(), expected);

        // Only intersecting because of the border.
        render::render_layout(saver, layout, geom::Rect{-1, -1, 1, 1});
        a.expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{20, 40, 1, 1});
        a.expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{20, 0, 1, 1});
        a.expect_eq(saver.take_commands(), expected);
        render::render_layout(saver, layout, geom::Rect{0, 40, 1, 1});
        a.expect_eq(saver.take_commands(), expected);

        // Non-intersecting cull rects.
        render::render_layout(saver, layout, geom::Rect{0, 41, 1, 1});
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});
        render::render_layout(saver, layout, geom::Rect{21, 41, 1, 1});
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});
        render::render_layout(saver, layout, geom::Rect{21, -1, 1, 1});
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});
        render::render_layout(saver, layout, geom::Rect{-2, -2, 1, 1});
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});
    });

    s.add_test("special backgrounds", [](etest::IActions &a) {
        dom::Node dom = dom::Element{.name{"html"}, .children{dom::Element{"body"}}};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{css::PropertyId::Display, "block"}},
                .children{style::StyledNode{
                        .node = std::get<dom::Element>(dom).children[0],
                        .properties = {{css::PropertyId::Display, "block"}},
                }},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .dimensions = {{0, 0, 20, 40}},
                .children = {{&styled.children[0], {{0, 0, 10, 10}}}},
        };

        gfx::CanvasCommandSaver saver;

        // No special backgrounds.
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(), CanvasCommands{gfx::ClearCmd{{0xFF, 0xFF, 0xFF}}});

        // Body background.
        styled.children[0].properties.emplace_back(css::PropertyId::BackgroundColor, "#abc");
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(),
                CanvasCommands{
                        gfx::ClearCmd{{0xAA, 0xBB, 0xCC}},
                        gfx::DrawRectCmd{{0, 0, 10, 10}, {0xAA, 0xBB, 0xCC}},
                });

        // Html background.
        styled.properties.emplace_back(css::PropertyId::BackgroundColor, "#123");
        render::render_layout(saver, layout);
        a.expect_eq(saver.take_commands(),
                CanvasCommands{
                        gfx::ClearCmd{{0x11, 0x22, 0x33}},
                        gfx::DrawRectCmd{{0, 0, 20, 40}, {0x11, 0x22, 0x33}},
                        gfx::DrawRectCmd{{0, 0, 10, 10}, {0xAA, 0xBB, 0xCC}},
                });
    });

    return s.run();
}
