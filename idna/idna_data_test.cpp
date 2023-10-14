// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "idna/idna_data.h"

#include "etest/etest2.h"

using Entry = decltype(idna::uts46::kMappings)::value_type;

// https://www.unicode.org/reports/tr46/#IDNA_Mapping_Table
int main() {
    etest::Suite s{};

    s.add_test("everything before ascii '-' is ~disallowed", [](etest::IActions &a) {
        a.expect_eq(idna::uts46::kMappings.at(0), Entry{char32_t{0}, idna::uts46::DisallowedStd3Valid{}});
        a.expect_eq(idna::uts46::kMappings.at(1), Entry{char32_t{'-'}, idna::uts46::Valid{}});
    });

    return s.run();
}
