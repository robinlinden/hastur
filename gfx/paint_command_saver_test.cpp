// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/paint_command_saver.h"

#include "etest/etest.h"

using namespace gfx;

using etest::expect_eq;

using PaintCommands = std::vector<PaintCommand>;

int main() {
    etest::test("PaintCommandSaver::take_commands", [] {
        PaintCommandSaver saver;
        expect_eq(saver.take_commands(), PaintCommands{});

        saver.set_scale(1);
        expect_eq(saver.take_commands(), PaintCommands{SetScaleCmd{1}});
        expect_eq(saver.take_commands(), PaintCommands{});

        saver.set_scale(1);
        saver.set_scale(1);
        expect_eq(saver.take_commands(), PaintCommands{SetScaleCmd{1}, SetScaleCmd{1}});
        expect_eq(saver.take_commands(), PaintCommands{});
    });

    etest::test("PaintCommandSaver::set_viewport_size", [] {
        PaintCommandSaver saver;
        saver.set_viewport_size(5, 15);
        expect_eq(saver.take_commands(), PaintCommands{SetViewportSizeCmd{5, 15}});
    });

    etest::test("PaintCommandSaver::set_scale", [] {
        PaintCommandSaver saver;
        saver.set_scale(1000);
        expect_eq(saver.take_commands(), PaintCommands{SetScaleCmd{1000}});
    });

    etest::test("PaintCommandSaver::add_translation", [] {
        PaintCommandSaver saver;
        saver.add_translation(-10, 10);
        expect_eq(saver.take_commands(), PaintCommands{AddTranslationCmd{-10, 10}});
    });

    etest::test("PaintCommandSaver::fill_rect", [] {
        PaintCommandSaver saver;
        saver.fill_rect({1, 2, 3, 4}, {0xab, 0xcd, 0xef});
        expect_eq(saver.take_commands(), PaintCommands{FillRectCmd{{1, 2, 3, 4}, {0xab, 0xcd, 0xef}}});
    });

    etest::test("replay_commands", [] {
        PaintCommandSaver saver;
        saver.set_scale(10);
        saver.set_scale(5);
        saver.set_viewport_size(1, 2);
        saver.set_scale(1);
        saver.add_translation(1234, 5678);
        saver.fill_rect({9, 9, 9, 9}, {0x12, 0x34, 0x56});
        auto cmds = saver.take_commands();

        PaintCommandSaver replayed;
        replay_commands(replayed, cmds);

        expect_eq(cmds, replayed.take_commands());
    });

    return etest::run_all_tests();
}
