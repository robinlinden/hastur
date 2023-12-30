// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/character_reference.h"

#include "etest/etest.h"

#include <string_view>

using namespace std::literals;

using etest::expect;
using etest::require;

using namespace html2;

int main() {
    etest::test("no entity found", [] {
        auto ref = find_named_character_reference_for("A"sv);
        expect(!ref.has_value());
    });

    etest::test("single-codepoint entity", [] {
        auto ref = find_named_character_reference_for("&lt"sv);
        require(ref.has_value());
        expect(ref->name == "&lt"sv);
        expect(ref->first_codepoint == '<');
        expect(!ref->second_codepoint.has_value());
    });

    etest::test("double-codepoint entity", [] {
        auto ref = find_named_character_reference_for("&NotSucceedsEqual;"sv);
        require(ref.has_value());
        expect(ref->name == "&NotSucceedsEqual;"sv);
        expect(ref->first_codepoint == 0x02AB0u);
        expect(ref->second_codepoint == 0x00338u);
    });

    etest::test("longest prefix is chosen", [] {
        auto ref = find_named_character_reference_for("&lt;"sv);
        require(ref.has_value());
        expect(ref->name == "&lt;"sv); // And not &lt which also matches.
    });

    etest::test("extra characters are ignored", [] {
        auto ref = find_named_character_reference_for("&lt;&lt;&abc;123"sv);
        require(ref.has_value());
        expect(ref->name == "&lt;"sv);
    });

    return etest::run_all_tests();
}
