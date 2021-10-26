// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/tokenizer.h"

#include "etest/etest.h"

#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;
using etest::expect;
using etest::require;

namespace {
bool contains_in_sequence(std::vector<html2::Token> const &haystack, std::vector<html2::Token> const &needles) {
    auto needle = cbegin(needles);
    for (auto const &candidate : haystack) {
        if (candidate == *needle) {
            ++needle;
        }
        if (needle == cend(needles)) {
            return true;
        }
    }
    return false;
}
} // namespace

int main() {
    etest::test("simple_page", [] {
        std::ifstream page{"html2/test/simple_page.html", std::ios::binary};
        require(page.is_open());
        std::string page_str{std::istreambuf_iterator<char>{page}, std::istreambuf_iterator<char>{}};
        std::vector<html2::Token> tokens;
        html2::Tokenizer tokenizer{page_str, [&](html2::Token &&t) {
                                       tokens.push_back(std::move(t));
                                   }};
        tokenizer.run();

        expect(contains_in_sequence(tokens,
                {html2::DoctypeToken{.name = "html"s},
                        html2::StartTagToken{.tag_name = "html"s},
                        html2::EndTagToken{.tag_name = "html"s}}));
    });

    return etest::run_all_tests();
}
