// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/canvas_command_saver.h"

#include "etest/etest.h"

#include <string>
#include <string_view>

using namespace gfx;
using namespace std::literals;

using etest::expect_eq;

using CanvasCommands = std::vector<CanvasCommand>;

int main() {
    etest::test("CanvasCommandSaver::take_commands", [] {
        CanvasCommandSaver saver;
        expect_eq(saver.take_commands(), CanvasCommands{});

        saver.set_scale(1);
        expect_eq(saver.take_commands(), CanvasCommands{SetScaleCmd{1}});
        expect_eq(saver.take_commands(), CanvasCommands{});

        saver.set_scale(1);
        saver.set_scale(1);
        expect_eq(saver.take_commands(), CanvasCommands{SetScaleCmd{1}, SetScaleCmd{1}});
        expect_eq(saver.take_commands(), CanvasCommands{});
    });

    etest::test("CanvasCommandSaver::set_viewport_size", [] {
        CanvasCommandSaver saver;
        saver.set_viewport_size(5, 15);
        expect_eq(saver.take_commands(), CanvasCommands{SetViewportSizeCmd{5, 15}});
    });

    etest::test("CanvasCommandSaver::set_scale", [] {
        CanvasCommandSaver saver;
        saver.set_scale(1000);
        expect_eq(saver.take_commands(), CanvasCommands{SetScaleCmd{1000}});
    });

    etest::test("CanvasCommandSaver::add_translation", [] {
        CanvasCommandSaver saver;
        saver.add_translation(-10, 10);
        expect_eq(saver.take_commands(), CanvasCommands{AddTranslationCmd{-10, 10}});
    });

    etest::test("CanvasCommandSaver::fill_rect", [] {
        CanvasCommandSaver saver;
        saver.fill_rect({1, 2, 3, 4}, {0xab, 0xcd, 0xef});
        expect_eq(saver.take_commands(), CanvasCommands{FillRectCmd{{1, 2, 3, 4}, {0xab, 0xcd, 0xef}}});
    });

    etest::test("CanvasCommandSaver::draw_border", [] {
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

        saver.draw_border({1, 2, 3, 4}, borders);
        expect_eq(saver.take_commands(), CanvasCommands{DrawBorderCmd{{1, 2, 3, 4}, borders}});
    });

    etest::test("CanvasCommandSaver::draw_text", [] {
        CanvasCommandSaver saver;
        saver.draw_text({1, 2}, "hello!"sv, {"comic sans"}, {11}, {1, 2, 3});
        expect_eq(saver.take_commands(), CanvasCommands{DrawTextCmd{{1, 2}, "hello!"s, {"comic sans"}, 11, {1, 2, 3}}});
    });

    etest::test("replay_commands", [] {
        CanvasCommandSaver saver;
        saver.set_scale(10);
        saver.set_scale(5);
        saver.set_viewport_size(1, 2);
        saver.set_scale(1);
        saver.add_translation(1234, 5678);
        saver.fill_rect({9, 9, 9, 9}, {0x12, 0x34, 0x56});
        saver.draw_border({9, 9, 9, 9}, {});
        saver.draw_text({10, 10}, "beep beep boop!"sv, {"helvetica"}, {42}, {3, 2, 1});
        auto cmds = saver.take_commands();

        CanvasCommandSaver replayed;
        replay_commands(replayed, cmds);

        expect_eq(cmds, replayed.take_commands());
    });

    return etest::run_all_tests();
}
