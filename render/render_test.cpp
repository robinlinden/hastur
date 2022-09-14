// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
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

int main() {
    etest::test("render simple layout", [] {
        dom::Node dom = dom::Element{"span", {}, {dom::Text{"hello"}}};

        auto const &children = std::get<dom::Element>(dom).children;
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{"display", "inline"}},
                .children = {{children[0], {{"display", "inline"}}, {}}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Inline,
                .dimensions = {},
                .children = {{&styled.children[0], layout::LayoutType::Inline, {}, {}}},
        };

        gfx::CanvasCommandSaver saver;
        gfx::Painter painter{saver};
        render::render_layout(painter, layout);

        expect_eq(saver.take_commands(), CanvasCommands{gfx::DrawTextCmd{{0, 0}, "hello", "arial", 10}});
    });

    etest::test("render block with background-color", [] {
        dom::Node dom = dom::Element{"div", {}, {dom::Element{"first"}}};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{"display", "block"}, {"background-color", "#0A0B0C"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Block,
                .dimensions = {{10, 20, 100, 100}, {}, {}, {}},
        };

        gfx::CanvasCommandSaver saver;
        gfx::Painter painter{saver};
        render::render_layout(painter, layout);

        geom::Rect expected_rect{10, 20, 100, 100};
        gfx::Color expected_color{0xA, 0xB, 0xC};

        expect_eq(saver.take_commands(), CanvasCommands{gfx::DrawRectCmd{expected_rect, expected_color, {}}});
    });

    etest::test("render block with transparent background-color", [] {
        dom::Node dom = dom::Element{"div", {}, {dom::Element{"first"}}};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{"display", "block"}, {"background-color", "transparent"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Block,
                .dimensions = {{10, 20, 100, 100}, {}, {}, {}},
        };

        gfx::CanvasCommandSaver saver;
        gfx::Painter painter{saver};
        render::render_layout(painter, layout);

        expect_eq(saver.take_commands(), CanvasCommands{});
    });

    etest::test("render block with borders, default color", [] {
        dom::Node dom = dom::Element{"div", {}, {dom::Element{"first"}}};
        auto styled = style::StyledNode{
                .node = dom,
                .properties = {{"display", "block"}, {"background-color", "#0A0B0C"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Block,
                .dimensions = {{0, 0, 20, 40}, {}, {10, 10, 10, 10}, {}},
        };

        gfx::CanvasCommandSaver saver;
        gfx::Painter painter{saver};
        render::render_layout(painter, layout);

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
                .properties = {{"display", "block"},
                        {"border-left-color", "#010101"},
                        {"border-right-color", "#020202"},
                        {"border-top-color", "#030303"},
                        {"border-bottom-color", "#040404"}},
        };

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Block,
                .dimensions = {{0, 0, 20, 40}, {}, {2, 4, 6, 8}, {}},
        };

        gfx::CanvasCommandSaver saver;
        gfx::Painter painter{saver};
        render::render_layout(painter, layout);

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
                .properties = {{"color", "#aabbcc"}},
                .children = {{children[0], {{"background-color", "currentcolor"}}, {}}},
        };
        styled.children[0].parent = &styled;

        auto layout = layout::LayoutBox{
                .node = &styled,
                .type = layout::LayoutType::Inline,
                .dimensions = {},
                .children = {{&styled.children[0], layout::LayoutType::Inline, {{0, 0, 20, 20}}, {}}},
        };

        gfx::CanvasCommandSaver saver;
        gfx::Painter painter{saver};
        render::render_layout(painter, layout);

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
        gfx::Painter painter{saver};

        // #rgba
        styled.properties = {{"background-color", "#abcd"}};
        auto cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{0xaa, 0xbb, 0xcc, 0xdd}}};
        render::render_layout(painter, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // #rrggbbaa
        styled.properties = {{"background-color", "#12345678"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{0x12, 0x34, 0x56, 0x78}}};
        render::render_layout(painter, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // #rgb
        styled.properties = {{"background-color", "#abc"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{0xaa, 0xbb, 0xcc}}};
        render::render_layout(painter, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});

        // #rrggbb
        styled.properties = {{"background-color", "#123456"}};
        cmd = gfx::DrawRectCmd{.rect{0, 0, 20, 20}, .color{gfx::Color{0x12, 0x34, 0x56}}};
        render::render_layout(painter, layout);
        expect_eq(saver.take_commands(), CanvasCommands{std::move(cmd)});
    });

    return etest::run_all_tests();
}
