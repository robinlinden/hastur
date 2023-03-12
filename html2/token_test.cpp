// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/token.h"

#include "etest/etest.h"

using etest::expect_eq;

using namespace html2;

int main() {
    etest::test("to_string(Doctype)", [] {
        expect_eq(to_string(DoctypeToken{.name = "test"}), R"(Doctype test "" "")");
        expect_eq(to_string(DoctypeToken{
                          .name = "html",
                          .public_identifier = "a",
                          .system_identifier = "b",
                  }),
                "Doctype html a b");
    });

    etest::test("to_string(StartTag)", [] {
        expect_eq(to_string(StartTagToken{.tag_name = "p", .self_closing = false}), "StartTag p 0");
        expect_eq(to_string(StartTagToken{.tag_name = "img", .self_closing = true}), "StartTag img 1");
    });

    etest::test("to_string(EndTag)", [] {
        expect_eq(to_string(EndTagToken{.tag_name = "p", .self_closing = false}), "EndTag p 0");
        expect_eq(to_string(EndTagToken{.tag_name = "img", .self_closing = true}), "EndTag img 1");
    });

    etest::test("to_string(Comment)", [] {
        expect_eq(to_string(CommentToken{"hello?"}), "Comment hello?");
        expect_eq(to_string(CommentToken{"!!!"}), "Comment !!!");
    });

    etest::test("to_string(Character)", [] {
        expect_eq(to_string(CharacterToken{'a'}), "Character a");
        expect_eq(to_string(CharacterToken{'?'}), "Character ?");
    });

    etest::test("to_string(EndOfFile)", [] { expect_eq(to_string(EndOfFileToken{}), "EndOfFile"); });

    return etest::run_all_tests();
}
