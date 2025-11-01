// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/token.h"

#include "etest/etest2.h"

using namespace html;

int main() {
    etest::Suite s;

    s.add_test("to_string(Doctype)", [](etest::IActions &a) {
        a.expect_eq(to_string(DoctypeToken{.name = "test"}), R"(Doctype test "" "")");
        a.expect_eq(to_string(DoctypeToken{
                            .name = "html",
                            .public_identifier = "a",
                            .system_identifier = "b",
                    }),
                "Doctype html a b");
    });

    s.add_test("to_string(StartTag)", [](etest::IActions &a) {
        a.expect_eq(to_string(StartTagToken{.tag_name = "p", .self_closing = false}), "StartTag p false");
        a.expect_eq(to_string(StartTagToken{.tag_name = "img", .self_closing = true}), "StartTag img true");
    });

    s.add_test("to_string(EndTag)", [](etest::IActions &a) {
        a.expect_eq(to_string(EndTagToken{.tag_name = "p"}), "EndTag p");
        a.expect_eq(to_string(EndTagToken{.tag_name = "img"}), "EndTag img");
    });

    s.add_test("to_string(Comment)", [](etest::IActions &a) {
        a.expect_eq(to_string(CommentToken{"hello?"}), "Comment hello?");
        a.expect_eq(to_string(CommentToken{"!!!"}), "Comment !!!");
    });

    s.add_test("to_string(Character)", [](etest::IActions &a) {
        a.expect_eq(to_string(CharacterToken{'a'}), "Character a");
        a.expect_eq(to_string(CharacterToken{'?'}), "Character ?");
    });

    s.add_test("to_string(EndOfFile)", [](etest::IActions &a) {
        a.expect_eq(to_string(EndOfFileToken{}), "EndOfFile"); //
    });

    return s.run();
}
