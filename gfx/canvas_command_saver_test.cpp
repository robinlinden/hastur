// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/canvas_command_saver.h"

#include "gfx/color.h"
#include "gfx/font.h"
#include "gfx/icanvas.h"

#include "etest/etest2.h"

#include <string>
#include <string_view>
#include <vector>

using namespace gfx;
using namespace std::literals;

using CanvasCommands = std::vector<CanvasCommand>;

int main() {
    etest::Suite s{};

    s.add_test("CanvasCommandSaver::take_commands", [](etest::IActions &a) {
        CanvasCommandSaver saver;
        a.expect_eq(saver.take_commands(), CanvasCommands{});

        saver.set_scale(1);
        a.expect_eq(saver.take_commands(), CanvasCommands{SetScaleCmd{1}});
        a.expect_eq(saver.take_commands(), CanvasCommands{});

        saver.set_scale(1);
        saver.set_scale(1);
        a.expect_eq(saver.take_commands(), CanvasCommands{SetScaleCmd{1}, SetScaleCmd{1}});
        a.expect_eq(saver.take_commands(), CanvasCommands{});
    });

    s.add_test("CanvasCommandSaver::set_viewport_size", [](etest::IActions &a) {
        CanvasCommandSaver saver;
        saver.set_viewport_size(5, 15);
        a.expect_eq(saver.take_commands(), CanvasCommands{SetViewportSizeCmd{5, 15}});
    });

    s.add_test("CanvasCommandSaver::set_scale", [](etest::IActions &a) {
        CanvasCommandSaver saver;
        saver.set_scale(1000);
        a.expect_eq(saver.take_commands(), CanvasCommands{SetScaleCmd{1000}});
    });

    s.add_test("CanvasCommandSaver::add_translation", [](etest::IActions &a) {
        CanvasCommandSaver saver;
        saver.add_translation(-10, 10);
        a.expect_eq(saver.take_commands(), CanvasCommands{AddTranslationCmd{-10, 10}});
    });

    s.add_test("CanvasCommandSaver::clear", [](etest::IActions &a) {
        CanvasCommandSaver saver;
        saver.clear({0xab, 0xcd, 0xef});
        a.expect_eq(saver.take_commands(), CanvasCommands{ClearCmd{{0xab, 0xcd, 0xef}}});
    });

    s.add_test("CanvasCommandSaver::draw_border", [](etest::IActions &a) {
        CanvasCommandSaver saver;

        Borders borders;
        borders.left.color = Color::from_rgb(0xFF00FF);
        borders.left.size = 10;
        borders.right.color = Color::from_rgb(0xFF00FF);
        borders.right.size = 10;
        borders.top.color = Color::from_rgb(0xFF00FF);
        borders.top.size = 20;
        borders.bottom.color = Color::from_rgb(0xFF00FF);
        borders.bottom.size = 10;

        Corners corners;
        corners.top_left = {1, 2};
        corners.top_right = {3, 4};
        corners.bottom_left = {5, 6};
        corners.bottom_right = {7, 8};

        saver.draw_rect({1, 2, 3, 4}, {0xFF, 0xAA, 0xFF}, borders, corners);
        a.expect_eq(
                saver.take_commands(), CanvasCommands{DrawRectCmd{{1, 2, 3, 4}, {0xFF, 0xAA, 0xFF}, borders, corners}});
    });

    s.add_test("CanvasCommandSaver::draw_text", [](etest::IActions &a) {
        CanvasCommandSaver saver;
        saver.draw_text({1, 2}, "hello!"sv, {"comic sans"}, {11}, {}, {1, 2, 3});
        a.expect_eq(saver.take_commands(),
                CanvasCommands{DrawTextCmd{{1, 2}, "hello!"s, {"comic sans"}, 11, {}, {1, 2, 3}}});

        saver.draw_text({1, 2}, "hello!"sv, std::vector<gfx::Font>{{"comic sans"}}, {11}, {}, {1, 2, 3});
        a.expect_eq(saver.take_commands(),
                CanvasCommands{DrawTextWithFontOptionsCmd{{1, 2}, "hello!"s, {{"comic sans"}}, 11, {}, {1, 2, 3}}});
    });

    s.add_test("CanvasCommandSaver::draw_pixels", [](etest::IActions &a) {
        CanvasCommandSaver saver;
        saver.draw_pixels({1, 2, 3, 4}, {{0x12, 0x34, 0x56, 0x78}});
        a.expect_eq(saver.take_commands(), CanvasCommands{DrawPixelsCmd{{1, 2, 3, 4}, {0x12, 0x34, 0x56, 0x78}}});
    });

    s.add_test("replay_commands", [](etest::IActions &a) {
        CanvasCommandSaver saver;
        saver.clear(gfx::Color{});
        saver.set_scale(10);
        saver.set_scale(5);
        saver.set_viewport_size(1, 2);
        saver.set_scale(1);
        saver.add_translation(1234, 5678);
        saver.draw_rect({9, 9, 9, 9}, {0x12, 0x34, 0x56}, {}, {});
        saver.draw_rect({9, 9, 9, 9}, {0x10, 0x11, 0x12}, {}, {});
        saver.draw_text({10, 10}, "beep beep boop!"sv, {"helvetica"}, {42}, {.italic = true}, {3, 2, 1});
        saver.draw_text({1, 5}, "hello?"sv, {{{"font1"}, {"font2"}}}, {42}, {}, {1, 2, 3});
        saver.clear(gfx::Color{1, 2, 3});
        saver.draw_pixels({1, 2, 3, 4}, {{0x12, 0x34, 0x56, 0x78}});
        auto cmds = saver.take_commands();

        CanvasCommandSaver replayed;
        replay_commands(replayed, cmds);

        a.expect_eq(cmds, replayed.take_commands());
    });

    return s.run();
}
