// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/string.h"

#include "etest/etest.h"

using namespace std::literals;

using etest::expect;
using util::no_case_compare;

int main() {
    etest::test("no case compare", [] {
        expect(no_case_compare("word"sv, "word"sv));
        expect(no_case_compare("WORD"sv, "WORD"sv));
        expect(no_case_compare("word"sv, "WORD"sv));
        expect(no_case_compare("WORD"sv, "word"sv));
        expect(no_case_compare("Abc-Def_Ghi"sv, "aBc-DEf_gHi"sv));
        expect(no_case_compare("10 seconds"sv, "10 Seconds"sv));
        expect(no_case_compare("Abc $#@"sv, "ABC $#@"sv));
        expect(!no_case_compare(" word"sv, "word"sv));
        expect(!no_case_compare("word "sv, "word"sv));
        expect(!no_case_compare("word "sv, "woord"sv));
    });

    return etest::run_all_tests();
}
