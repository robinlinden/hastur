// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/parser_states.h"

#include "etest/etest.h"
#include "html2/tokenizer.h"

using etest::expect_eq;

using NodeVec = std::vector<dom::Node>;

namespace {
struct ParseResult {
    dom::Document document{};
};

struct ParseOptions {
    html::InsertionMode initial_insertion_mode{};
    bool scripting{false};
};

// TODO(robinlinden): This is very awkward, but I'll make it better later, I promise.
ParseResult parse(std::string_view html, ParseOptions opts) {
    html2::Tokenizer tokenizer{html, [&](auto &, auto const &) {
                               }};

    ParseResult res{};
    html::InsertionMode mode{opts.initial_insertion_mode};
    std::stack<dom::Element *> open_elements{};
    html::Actions actions{res.document, tokenizer, opts.scripting, open_elements};

    auto on_token = [&](html2::Tokenizer &, html2::Token const &token) {
        mode = std::visit([&](auto &v) { return v.process(actions, token); }, mode).value_or(mode);
    };

    tokenizer = html2::Tokenizer{html, std::move(on_token)};
    tokenizer.run();
    return res;
}

void initial_tests() {
    etest::test("Initial: whitespace before doctype", [] {
        auto res = parse("    <!DOCTYPE html>", {});
        expect_eq(res.document.doctype, "html");
        res = parse("\t\n\r <!DOCTYPE bad>", {});
        expect_eq(res.document.doctype, "bad");
    });
}

void before_html_tests() {
    etest::test("BeforeHtml: html tag", [] {
        auto res = parse("<html foo='bar'>", {});
        expect_eq(res.document.html(), dom::Element{"html", {{"foo", "bar"}}, {dom::Element{"head"}}});
    });

    etest::test("BeforeHtml: boring whitespace before html is dropped", [] {
        auto res = parse("<!DOCTYPE asdf>\t\n\f\r <html foo='bar'>", {});
        expect_eq(res.document.html(), dom::Element{"html", {{"foo", "bar"}}, {dom::Element{"head"}}});
    });
}

void before_head_tests() {
    etest::test("BeforeHead: head tag", [] {
        auto res = parse("<head foo='bar'>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head", {{"foo", "bar"}}}}});
    });

    etest::test("BeforeHtml: boring whitespace before head is dropped", [] {
        auto res = parse("<html>\t\n\f\r <head foo='bar'>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head", {{"foo", "bar"}}}}});
    });
}

void in_head_tests() {
    etest::test("InHead: base, basefont, bgsound, link", [] {
        auto res = parse("<base> <basefont> <bgsound> <link>", {});

        auto head_children =
                NodeVec{dom::Element{"base"}, dom::Element{"basefont"}, dom::Element{"bgsound"}, dom::Element{"link"}};
        auto head = dom::Element{"head", {}, std::move(head_children)};

        expect_eq(res.document.html(), dom::Element{"html", {}, {std::move(head)}});
    });

    etest::test("InHead: meta", [] {
        auto res = parse("<meta>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head", {}, {dom::Element{"meta"}}}}});
    });

    etest::test("InHead: title", [] {
        auto res = parse("<title><body></title>", {});
        auto title = dom::Element{"title", {}, {dom::Text{"<body>"}}};
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(title)}}}});
    });

    etest::test("InHead: style", [] {
        auto res = parse("<style>p { color: green; }</style>", {});
        auto style = dom::Element{"style", {}, {dom::Text{"p { color: green; }"}}};
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(style)}}}});
    });

    etest::test("InHead: style, abrupt eof", [] {
        auto res = parse("<style>p { color: green; }", {});
        auto style = dom::Element{"style", {}, {dom::Text{"p { color: green; }"}}};
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(style)}}}});
    });

    etest::test("InHead: script", [] {
        auto res = parse("<script>totally.js()</script>", {});
        auto script = dom::Element{"script", {}, {dom::Text{"totally.js()"}}};
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(script)}}}});
    });

    etest::test("InHead: head end tag", [] {
        auto res = parse("</head>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}}});
    });
}

void after_head_tests() {
    // TODO(robinlinden): This is where this parser ends for now. :(
    etest::test("AfterHead: body", [] {
        auto res = parse("<body>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}}});
    });
}

} // namespace

int main() {
    initial_tests();
    before_html_tests();
    before_head_tests();
    in_head_tests();
    after_head_tests();
    return etest::run_all_tests();
}
