// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
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

using etest::expect_eq;
using etest::require;

using namespace html2;

namespace {
std::vector<Token> run_tokenizer(std::string_view input) {
    std::vector<Token> tokens;
    Tokenizer tokenizer{input, [&](Token &&t) {
                            tokens.push_back(std::move(t));
                        }};
    tokenizer.run();
    return tokens;
}
} // namespace

int main() {
    etest::test("simple_page", [] {
        std::ifstream page{"html2/test/simple_page.html", std::ios::binary};
        require(page.is_open());
        std::string page_str{std::istreambuf_iterator<char>{page}, std::istreambuf_iterator<char>{}};
        auto tokens = run_tokenizer(page_str);

        expect_eq(tokens,
                std::vector<Token>{DoctypeToken{.name = "html"s},
                        CharacterToken{'\n'},
                        StartTagToken{.tag_name = "html"s},
                        CharacterToken{'\n'},
                        EndTagToken{.tag_name = "html"s},
                        CharacterToken{'\n'},
                        EndOfFileToken{}});
    });

    etest::test("comment, simple", [] {
        auto tokens = run_tokenizer("<!-- Hello -->");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = " Hello "}, EndOfFileToken{}});
    });

    etest::test("comment, empty", [] {
        auto tokens = run_tokenizer("<!---->");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = ""}, EndOfFileToken{}});
    });

    etest::test("comment, with dashes and bang", [] {
        auto tokens = run_tokenizer("<!--!-->");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = "!"}, EndOfFileToken{}});
    });

    etest::test("comment, with new lines", [] {
        auto tokens = run_tokenizer("<!--\nOne\nTwo\n-->");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = "\nOne\nTwo\n"}, EndOfFileToken{}});
    });

    etest::test("comment, multiple with new lines", [] {
        auto tokens = run_tokenizer("<!--a-->\n<!--b-->\n<!--c-->");

        expect_eq(tokens,
                std::vector<Token>{CommentToken{.data = "a"},
                        CharacterToken{'\n'},
                        CommentToken{.data = "b"},
                        CharacterToken{'\n'},
                        CommentToken{.data = "c"},
                        EndOfFileToken{}});
    });

    etest::test("comment, allowed to end with <!", [] {
        auto tokens = run_tokenizer("<!--My favorite operators are > and <!-->");

        expect_eq(tokens,
                std::vector<Token>{CommentToken{.data = "My favorite operators are > and <!"}, EndOfFileToken{}});
    });

    etest::test("comment, nested comment", [] {
        auto tokens = run_tokenizer("<!--<!---->");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = "<!--"}, EndOfFileToken{}});
    });

    etest::test("comment, nested comment closed", [] {
        auto tokens = run_tokenizer("<!-- <!-- nested --> -->");

        expect_eq(tokens,
                std::vector<Token>{CommentToken{.data = " <!-- nested "},
                        CharacterToken{' '},
                        CharacterToken{'-'},
                        CharacterToken{'-'},
                        CharacterToken{'>'},
                        EndOfFileToken{}});
    });

    etest::test("comment, abrupt closing in comment start", [] {
        auto tokens = run_tokenizer("<!-->");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = ""}, EndOfFileToken{}});
    });

    etest::test("comment, abrupt closing in comment start dash", [] {
        auto tokens = run_tokenizer("<!--->");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = ""}, EndOfFileToken{}});
    });

    etest::test("comment, incorrectly closed comment", [] {
        auto tokens = run_tokenizer("<!--abc--!>");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = "abc"}, EndOfFileToken{}});
    });

    etest::test("comment, end before comment", [] {
        auto tokens = run_tokenizer("<!--");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = ""}, EndOfFileToken{}});
    });

    etest::test("comment, eof before comment is closed", [] {
        auto tokens = run_tokenizer("<!--abc");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = "abc"}, EndOfFileToken{}});
    });

    etest::test("character entity reference, simple", [] {
        auto tokens = run_tokenizer("&lt;");
        expect_eq(tokens, std::vector<Token>{CharacterToken{'<'}, EndOfFileToken{}});
    });

    etest::test("character entity reference, only &", [] {
        auto tokens = run_tokenizer("&");
        expect_eq(tokens, std::vector<Token>{CharacterToken{'&'}, EndOfFileToken{}});
    });

    etest::test("character entity reference, not ascii alphanumeric", [] {
        auto tokens = run_tokenizer("&@");
        expect_eq(tokens, std::vector<Token>{CharacterToken{'&'}, CharacterToken{'@'}, EndOfFileToken{}});
    });

    return etest::run_all_tests();
}
