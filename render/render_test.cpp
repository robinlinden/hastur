// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "render/render.h"

#include "dom/dom.h"
#include "etest/etest.h"
#include "gfx/canvas_command_saver.h"
#include "layout/layout.h"
#include "style/styled_node.h"

using etest::expect_eq;

using CanvasCommands = std::vector<gfx::CanvasCommand>;

int main() {
    etest::test("render simple layout", [] {
        auto dom = dom::create_element_node("span", {}, {dom::create_text_node("hello")});

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

    return etest::run_all_tests();
}
