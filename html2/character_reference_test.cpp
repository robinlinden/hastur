// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/character_reference.h"

#include "etest/etest2.h"

#include <string_view>

using namespace std::literals;

using namespace html2;

int main() {
    etest::Suite s;
    s.add_test("no entity found", [](etest::IActions &a) {
        auto ref = find_named_character_reference_for("A"sv);
        a.expect(!ref.has_value());
    });

    s.add_test("single-codepoint entity", [](etest::IActions &a) {
        auto ref = find_named_character_reference_for("&lt"sv);
        a.require(ref.has_value());
        a.expect(ref->name == "&lt"sv);
        a.expect(ref->first_codepoint == '<');
        a.expect(!ref->second_codepoint.has_value());
    });

    s.add_test("double-codepoint entity", [](etest::IActions &a) {
        auto ref = find_named_character_reference_for("&NotSucceedsEqual;"sv);
        a.require(ref.has_value());
        a.expect(ref->name == "&NotSucceedsEqual;"sv);
        a.expect(ref->first_codepoint == 0x02AB0u);
        a.expect(ref->second_codepoint == 0x00338u);
    });

    s.add_test("longest prefix is chosen", [](etest::IActions &a) {
        auto ref = find_named_character_reference_for("&lt;"sv);
        a.require(ref.has_value());
        a.expect(ref->name == "&lt;"sv); // And not &lt which also matches.
    });

    s.add_test("extra characters are ignored", [](etest::IActions &a) {
        auto ref = find_named_character_reference_for("&lt;&lt;&abc;123"sv);
        a.require(ref.has_value());
        a.expect(ref->name == "&lt;"sv);
    });

    return s.run();
}
