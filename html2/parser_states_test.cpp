// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/parser_states.h"

#include "html2/tokenizer.h"

#include "etest/etest.h"
#include "html/parser_actions.h"

using etest::expect_eq;

using NodeVec = std::vector<dom::Node>;

namespace {
struct ParseResult {
    dom::Document document{};
};

struct ParseOptions {
    html2::InsertionMode initial_insertion_mode{};
    bool scripting{false};
};

// TODO(robinlinden): This is very awkward, but I'll make it better later, I promise.
ParseResult parse(std::string_view html, ParseOptions opts) {
    html2::Tokenizer tokenizer{html, [&](auto &, auto const &) {
                               }};

    ParseResult res{};
    html2::InsertionMode mode{opts.initial_insertion_mode};
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

    etest::test("Initial: comment", [] {
        auto res = parse("<!-- hello --><!DOCTYPE html>", {});
        expect_eq(res.document.doctype, "html");
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}}});
    });

    etest::test("Initial: doctype, sane", [] {
        auto res = parse("<!DOCTYPE html>", {});
        expect_eq(res.document.doctype, "html");
        expect_eq(res.document.mode, dom::Document::Mode::NoQuirks);
    });

    etest::test("Initial: doctype, sane-ish", [] {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01">)", {});
        expect_eq(res.document.mode, dom::Document::Mode::NoQuirks);
    });

    etest::test("Initial: doctype, also sane-ish", [] {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "def" "abc">)", {});
        expect_eq(res.document.mode, dom::Document::Mode::NoQuirks);
    });

    etest::test("Initial: doctype, quirky 0", [] {
        auto res = parse("<!DOCTYPE is_this_the_abyss?>", {});
        expect_eq(res.document.doctype, "is_this_the_abyss?");
        expect_eq(res.document.mode, dom::Document::Mode::Quirks);
    });

    etest::test("Initial: doctype, quirky 1", [] {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 FRAMESET//">)", {});
        expect_eq(res.document.mode, dom::Document::Mode::Quirks);
    });

    etest::test("Initial: doctype, quirky 2", [] {
        auto res = parse("<!DOCTYPE html SYSTEM http://www.IBM.com/data/dtd/v11/ibmxhtml1-transitional.dtd>", {});
        expect_eq(res.document.mode, dom::Document::Mode::Quirks);
    });

    etest::test("Initial: doctype, quirky 3", [] {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "HTML">)", {});
        expect_eq(res.document.mode, dom::Document::Mode::Quirks);
    });

    etest::test("Initial: doctype, quirky 4", [] {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "-//sun microsystems corp.//dtd hotjava html// i love this">)", {});
        expect_eq(res.document.mode, dom::Document::Mode::Quirks);
    });

    etest::test("Initial: doctype, quirky-ish 0", [] {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "-//w3c//dtd xhtml 1.0 transitional//hello">)", {});
        expect_eq(res.document.mode, dom::Document::Mode::LimitedQuirks);
    });

    etest::test("Initial: doctype, quirky-ish 1", [] {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 FRAMESET//" "">)", {});
        expect_eq(res.document.mode, dom::Document::Mode::LimitedQuirks);
    });
}

void before_html_tests() {
    etest::test("BeforeHtml: comment", [] {
        auto res = parse("<!DOCTYPE html><!-- hello --><html foo='bar'>", {});
        expect_eq(res.document.html(), dom::Element{"html", {{"foo", "bar"}}, {dom::Element{"head"}}});
    });

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
    etest::test("BeforeHead: comment", [] {
        auto res = parse("<html><!-- comment --><head foo='bar'>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head", {{"foo", "bar"}}}}});
    });

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
    etest::test("InHead: comment", [] {
        auto res = parse("<html><head><!-- comment --><meta>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head", {}, {dom::Element{"meta"}}}}});
    });

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
        auto res = parse("<title><body>&amp;</title>", {});
        auto title = dom::Element{"title", {}, {dom::Text{"<body>&"}}};
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

void in_head_noscript_tests() {
    etest::test("InHeadNoscript: doctype is ignored", [] {
        auto res = parse("<noscript><!doctype html></noscript>", {});
        auto const &html = res.document.html();
        expect_eq(html, dom::Element{"html", {}, {dom::Element{"head", {}, {dom::Element{"noscript"}}}}});
    });

    etest::test("InHeadNoscript: html attributes are reparented", [] {
        auto res = parse("<html foo=bar><noscript><html foo=baz hello=world>", {});
        auto const &head = std::get<dom::Element>(res.document.html().children.at(0));
        expect_eq(res.document.html().attributes, dom::AttrMap{{"foo", "bar"}, {"hello", "world"}});
        expect_eq(head, dom::Element{"head", {}, {dom::Element{"noscript"}}});
    });

    etest::test("InHeadNoScript: style", [] {
        auto res = parse("<noscript><style>p { color: green; }", {});
        auto noscript = dom::Element{"noscript", {}, {dom::Element{"style", {}, {dom::Text{"p { color: green; }"}}}}};
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(noscript)}}}});
    });

    etest::test("InHeadNoScript: br", [] {
        auto res = parse("<noscript></br>", {});
        auto noscript = dom::Element{"noscript"};
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(noscript)}}}});
    });

    etest::test("InHeadNoScript: noscript", [] {
        auto res = parse("<noscript><noscript>", {});
        auto noscript = dom::Element{"noscript"};
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(noscript)}}}});
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
    in_head_noscript_tests();
    after_head_tests();
    return etest::run_all_tests();
}
