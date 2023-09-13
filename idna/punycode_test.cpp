// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "idna/punycode.h"

#include "etest/etest2.h"
#include "util/unicode.h"

namespace {
std::string unicode_as_utf8_string(std::vector<int> const &code_points) {
    std::string result{};
    for (auto const code_point : code_points) {
        result += util::unicode_to_utf8(code_point);
    }

    return result;
}
} // namespace

int main() {
    etest::Suite s{};

    // https://datatracker.ietf.org/doc/html/rfc3492#section-7
    s.add_test("(A) Arabic (Egyptian)", [](etest::IActions &a) {
        // u+0644 u+064A u+0647 u+0645 u+0627 u+0628 u+062A u+0643 u+0644
        // u+0645 u+0648 u+0634 u+0639 u+0631 u+0628 u+064A u+061F
        // Punycode: egbpdaj6bu4bxfgehfvwxn
        std::string expected = unicode_as_utf8_string({0x0644,
                0x064A,
                0x0647,
                0x0645,
                0x0627,
                0x0628,
                0x062A,
                0x0643,
                0x0644,
                0x0645,
                0x0648,
                0x0634,
                0x0639,
                0x0631,
                0x0628,
                0x064A,
                0x061F});
        a.expect_eq(idna::Punycode::to_utf8("egbpdaj6bu4bxfgehfvwxn").value(), expected);
    });

    s.add_test("(M) <amuro><namie>-with-SUPER-MONKEYS", [](etest::IActions &a) {
        // u+5B89 u+5BA4 u+5948 u+7F8E u+6075 u+002D u+0077 u+0069 u+0074
        // u+0068 u+002D U+0053 U+0055 U+0050 U+0045 U+0052 u+002D U+004D
        // U+004F U+004E U+004B U+0045 U+0059 U+0053
        // Punycode: -with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n
        std::string expected = unicode_as_utf8_string({0x5B89,
                0x5BA4,
                0x5948,
                0x7F8E,
                0x6075,
                '-',
                'w',
                'i',
                't',
                'h',
                '-',
                'S',
                'U',
                'P',
                'E',
                'R',
                '-',
                'M',
                'O',
                'N',
                'K',
                'E',
                'Y',
                'S'});
        a.expect_eq(idna::Punycode::to_utf8("-with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n").value(), expected);
    });

    s.add_test("(P) Maji<de>Koi<suru>5<byou><mae>", [](etest::IActions &a) {
        // U+004D u+0061 u+006A u+0069 u+3067 U+004B u+006F u+0069 u+3059
        // u+308B u+0035 u+79D2 u+524D
        // Punycode: MajiKoi5-783gue6qz075azm5e
        std::string expected = unicode_as_utf8_string(
                {'M', 'a', 'j', 'i', 0x3067, 'K', 'o', 'i', 0x3059, 0x308B, '5', 0x79D2, 0x524D});
        a.expect_eq(idna::Punycode::to_utf8("MajiKoi5-783gue6qz075azm5e").value(), expected);
    });

    // Error handling.
    s.add_test("non-ascii before separator", [](etest::IActions &a) {
        a.expect_eq(idna::Punycode::to_utf8("\xF0-").has_value(), false); //
    });

    s.add_test("out of data", [](etest::IActions &a) {
        a.expect_eq(idna::Punycode::to_utf8("-3").has_value(), false); //
    });

    s.add_test("non-ascii after separator", [](etest::IActions &a) {
        a.expect_eq(idna::Punycode::to_utf8("-\xF0").has_value(), false); //
    });

    // Other functionality.
    s.add_test("uppercase punycode", [](etest::IActions &a) {
        // Same as (P) Maji<de>Koi<suru>5<byou><mae>, but with the punycode capitalized.
        std::string expected = unicode_as_utf8_string(
                {'M', 'a', 'j', 'i', 0x3067, 'K', 'o', 'i', 0x3059, 0x308B, '5', 0x79D2, 0x524D});
        a.expect_eq(idna::Punycode::to_utf8("MajiKoi5-783GUE6QZ075AZM5E").value(), expected);
    });

    return s.run();
}
