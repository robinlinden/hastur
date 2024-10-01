// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/parser_states.h"

#include "dom/dom.h"
#include "html2/token.h"
#include "html2/tokenizer.h"

#include "etest/etest.h"
#include "html/parser_actions.h"

#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

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
    std::vector<dom::Element *> open_elements{};
    html::Actions actions{res.document, tokenizer, opts.scripting, mode, open_elements};

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
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
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
    etest::test("BeforeHtml: doctype", [] {
        auto res = parse("<!DOCTYPE html>", {.initial_insertion_mode = html2::BeforeHtml{}});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("BeforeHtml: comment", [] {
        auto res = parse("<!DOCTYPE html><!-- hello --><html foo='bar'>", {});
        expect_eq(res.document.html(),
                dom::Element{"html", {{"foo", "bar"}}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("BeforeHtml: html tag", [] {
        auto res = parse("<html foo='bar'>", {});
        expect_eq(res.document.html(),
                dom::Element{"html", {{"foo", "bar"}}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("BeforeHtml: boring whitespace before html is dropped", [] {
        auto res = parse("<!DOCTYPE asdf>\t\n\f\r <html foo='bar'>", {});
        expect_eq(res.document.html(),
                dom::Element{"html", {{"foo", "bar"}}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("BeforeHtml: head end-tag", [] {
        auto res = parse("</head>", {.initial_insertion_mode = html2::BeforeHtml{}});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("BeforeHtml: dropped end-tag", [] {
        auto res = parse("</img>", {.initial_insertion_mode = html2::BeforeHtml{}});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });
}

void before_head_tests() {
    etest::test("BeforeHead: comment", [] {
        auto res = parse("<html><!-- comment --><head foo='bar'>", {});
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {{"foo", "bar"}}}, dom::Element{"body"}}});
    });

    etest::test("BeforeHead: doctype", [] {
        auto res = parse("<html><!DOCTYPE html><head foo='bar'>", {});
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {{"foo", "bar"}}}, dom::Element{"body"}}});
    });

    etest::test("BeforeHead: html tag", [] {
        auto res = parse("<html foo=bar><html foo=baz hello=world>", {});
        auto const &head = std::get<dom::Element>(res.document.html().children.at(0));
        expect_eq(res.document.html().attributes, dom::AttrMap{{"foo", "bar"}, {"hello", "world"}});
        expect_eq(head, dom::Element{"head"});
    });

    etest::test("BeforeHead: head tag", [] {
        auto res = parse("<head foo='bar'>", {});
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {{"foo", "bar"}}}, dom::Element{"body"}}});
    });

    etest::test("BeforeHead: end-tag fallthrough", [] {
        auto res = parse("</head>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("BeforeHead: ignored end-tag", [] {
        auto res = parse("</p><head foo=bar>", {});
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {{"foo", "bar"}}}, dom::Element{"body"}}});
    });

    etest::test("BeforeHtml: boring whitespace before head is dropped", [] {
        auto res = parse("<html>\t\n\f\r <head foo='bar'>", {});
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {{"foo", "bar"}}}, dom::Element{"body"}}});
    });
}

void in_head_tests() {
    etest::test("InHead: comment", [] {
        auto res = parse("<html><head><!-- comment --><meta>", {});
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {dom::Element{"meta"}}}, dom::Element{"body"}}});
    });

    etest::test("InHead: doctype", [] {
        auto res = parse("<head><!doctype HTML>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("InHead: end tag parse error", [] {
        auto res = parse("<head></p>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("InHead: html attributes are reparented", [] {
        auto res = parse("<html foo=bar><head><html foo=baz hello=world>", {});
        auto const &head = std::get<dom::Element>(res.document.html().children.at(0));
        expect_eq(res.document.html().attributes, dom::AttrMap{{"foo", "bar"}, {"hello", "world"}});
        expect_eq(head, dom::Element{"head"});
    });

    etest::test("InHead: base, basefont, bgsound, link", [] {
        auto res = parse("<base> <basefont> <bgsound> <link>", {});

        auto head_children =
                NodeVec{dom::Element{"base"}, dom::Element{"basefont"}, dom::Element{"bgsound"}, dom::Element{"link"}};
        auto head = dom::Element{"head", {}, std::move(head_children)};

        expect_eq(res.document.html(), dom::Element{"html", {}, {std::move(head), dom::Element{"body"}}});
    });

    etest::test("InHead: meta", [] {
        auto res = parse("<meta>", {});
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {dom::Element{"meta"}}}, dom::Element{"body"}}});
    });

    etest::test("InHead: title", [] {
        auto res = parse("<title><body>&amp;</title>", {});
        auto title = dom::Element{"title", {}, {dom::Text{"<body>&"}}};
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(title)}}, dom::Element{"body"}}});
    });

    etest::test("InHead: style", [] {
        auto res = parse("<style>p { color: green; }</style>", {});
        auto style = dom::Element{"style", {}, {dom::Text{"p { color: green; }"}}};
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(style)}}, dom::Element{"body"}}});
    });

    etest::test("InHead: style, abrupt eof", [] {
        auto res = parse("<style>p { color: green; }", {});
        auto style = dom::Element{"style", {}, {dom::Text{"p { color: green; }"}}};
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(style)}}, dom::Element{"body"}}});
    });

    etest::test("InHead: script", [] {
        auto res = parse("<script>totally.js()</script>", {});
        auto script = dom::Element{"script", {}, {dom::Text{"totally.js()"}}};
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(script)}}, dom::Element{"body"}}});
    });

    etest::test("InHead: head end tag", [] {
        auto res = parse("</head>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });
}

void in_head_noscript_tests() {
    etest::test("InHeadNoscript: doctype is ignored", [] {
        auto res = parse("<noscript><!doctype html></noscript>", {});
        auto const &html = res.document.html();
        expect_eq(html,
                dom::Element{"html", {}, {dom::Element{"head", {}, {dom::Element{"noscript"}}}, dom::Element{"body"}}});
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
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(noscript)}}, dom::Element{"body"}}});
    });

    etest::test("InHeadNoScript: style w/ end tags", [] {
        auto res = parse("<noscript><style>p { color: green; }</style></noscript>", {});
        auto noscript = dom::Element{"noscript", {}, {dom::Element{"style", {}, {dom::Text{"p { color: green; }"}}}}};
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(noscript)}}, dom::Element{"body"}}});
    });

    etest::test("InHeadNoScript: br", [] {
        auto res = parse("<noscript></br>", {});
        auto noscript = dom::Element{"noscript"};
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(noscript)}}, dom::Element{"body"}}});
    });

    etest::test("InHeadNoScript: noscript", [] {
        auto res = parse("<noscript><noscript>", {});
        auto noscript = dom::Element{"noscript"};
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(noscript)}}, dom::Element{"body"}}});
    });
}

void after_head_tests() {
    etest::test("AfterHead: boring whitespace", [] {
        auto res = parse("<head></head> ", {});
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head"}, dom::Text{" "}, dom::Element{"body"}}});
    });

    etest::test("AfterHead: comment", [] {
        auto res = parse("<head></head><!-- comment -->", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("AfterHead: doctype", [] {
        auto res = parse("<head></head><!doctype html>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("AfterHead: html", [] {
        auto res = parse("<html foo=bar><head></head><html foo=baz hello=world>", {});
        auto const &head = std::get<dom::Element>(res.document.html().children.at(0));
        expect_eq(res.document.html().attributes, dom::AttrMap{{"foo", "bar"}, {"hello", "world"}});
        expect_eq(head, dom::Element{"head"});
    });

    etest::test("AfterHead: body", [] {
        auto res = parse("<body>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("AfterHead: base, basefont, bgsound, link", [] {
        auto res = parse("<head></head><base><basefont><bgsound><link>", {});

        auto head_children =
                NodeVec{dom::Element{"base"}, dom::Element{"basefont"}, dom::Element{"bgsound"}, dom::Element{"link"}};
        auto head = dom::Element{"head", {}, std::move(head_children)};

        expect_eq(res.document.html(), dom::Element{"html", {}, {std::move(head), dom::Element{"body"}}});
    });

    etest::test("AfterHead: head", [] {
        auto res = parse("<head></head><head>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("AfterHead: </template>", [] {
        auto res = parse("<head></head></template>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("AfterHead: </body>", [] {
        auto res = parse("<head></head></body>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("AfterHead: </html>", [] {
        auto res = parse("<head></head></html>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("AfterHead: </br>", [] {
        auto res = parse("<head></head></br>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("AfterHead: </error>", [] {
        auto res = parse("<head></head></error>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    etest::test("AfterHead: <frameset>", [] {
        auto res = parse("<head></head><frameset>", {});
        expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"frameset"}}});
    });

    etest::test("AfterHead: <style>p { color: green; }", [] {
        auto res = parse("<head></head><style>p { color: green; }</style>", {});
        auto style = dom::Element{"style", {}, {dom::Text{"p { color: green; }"}}};
        expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(style)}}, dom::Element{"body"}}});
    });
}

void in_body_tests() {
    etest::test("InBody: null character", [] {
        auto res = parse("<body>\0"sv, {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        expect_eq(actual_body, dom::Element{"body"});
    });

    etest::test("InBody: boring whitespace", [] {
        auto res = parse("<body>\t"sv, {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        expect_eq(actual_body, dom::Element{"body", {}, {dom::Text{"\t"}}});
    });

    etest::test("InBody: character", [] {
        auto res = parse("<body>asdf"sv, {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        expect_eq(actual_body, dom::Element{"body", {}, {dom::Text{"asdf"}}});
    });

    etest::test("InBody: comment", [] {
        auto res = parse("<body><!-- comment -->", {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        expect_eq(actual_body, dom::Element{"body"});
    });

    etest::test("InBody: doctype", [] {
        auto res = parse("<body><!doctype html>", {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        expect_eq(actual_body, dom::Element{"body"});
    });

    etest::test("InBody: in-head-element", [] {
        auto res = parse("<body><title><html>&amp;</title>", {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        expect_eq(actual_body, dom::Element{"body", {}, {dom::Element{"title", {}, {dom::Text{"<html>&"}}}}});
    });

    etest::test("InBody: template end tag", [] {
        auto res = parse("<body></template>", {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        expect_eq(actual_body, dom::Element{"body"});
    });

    etest::test("InBody: automatically-closed p element", [] {
        auto res = parse("<body><p>hello<p>world", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        expect_eq(body,
                dom::Element{
                        "body",
                        {},
                        {
                                dom::Element{"p", {}, {dom::Text{"hello"}}},
                                dom::Element{"p", {}, {dom::Text{"world"}}},
                        },
                });
    });

    etest::test("InBody: automatically-closed p element, not current element", [] {
        auto res = parse("<body><p>hello<ruby><rb><p>world", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        expect_eq(body,
                dom::Element{
                        "body",
                        {},
                        {
                                dom::Element{"p",
                                        {},
                                        {
                                                dom::Text{"hello"},
                                                dom::Element{"ruby", {}, {dom::Element{"rb"}}},
                                        }},
                                dom::Element{"p", {}, {dom::Text{"world"}}},
                        },
                });
    });

    etest::test("InBody: <hr>", [] {
        auto res = parse("<body><p><hr>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        expect_eq(body, dom::Element{"body", {}, {dom::Element{"p"}, dom::Element{"hr"}}});
    });

    etest::test("InBody: <template> doesn't crash", [] {
        std::ignore = parse("<body><template>", {}); //
    });
}

void in_frameset_tests() {
    etest::test("InFrameset: boring whitespace", [] {
        auto res = parse("<head></head><frameset> ", {});
        dom::Element expected{
                "html",
                {},
                {
                        dom::Element{"head"},
                        dom::Element{"frameset", {}, {dom::Text{" "}}},
                },
        };
        expect_eq(res.document.html(), expected);
    });

    etest::test("InFrameset: comment", [] {
        auto res = parse("<head></head><frameset><!-- comment -->", {});
        dom::Element expected{
                "html",
                {},
                {dom::Element{"head"}, dom::Element{"frameset"}},
        };
        expect_eq(res.document.html(), expected);
    });

    etest::test("InFrameset: doctype", [] {
        auto res = parse("<head></head><frameset><!doctype html>", {});
        dom::Element expected{
                "html",
                {},
                {dom::Element{"head"}, dom::Element{"frameset"}},
        };
        expect_eq(res.document.html(), expected);
    });

    etest::test("InFrameset: <html>", [] {
        auto res = parse("<head></head><frameset><html foo=bar>", {});
        dom::Element expected{
                "html",
                {{"foo", "bar"}},
                {dom::Element{"head"}, dom::Element{"frameset"}},
        };
        expect_eq(res.document.html(), expected);
    });

    etest::test("InFrameset: <frameset>", [] {
        auto res = parse("<head></head><frameset><frameset>", {});
        dom::Element expected{
                "html",
                {},
                {
                        dom::Element{"head"},
                        dom::Element{"frameset", {}, {dom::Element{"frameset"}}},
                },
        };
        expect_eq(res.document.html(), expected);
    });

    etest::test("InFrameset: <frame>", [] {
        auto res = parse("<head></head><frameset><frame>", {});
        dom::Element expected{
                "html",
                {},
                {
                        dom::Element{"head"},
                        dom::Element{"frameset", {}, {dom::Element{"frame"}}},
                },
        };
        expect_eq(res.document.html(), expected);
    });

    etest::test("InFrameset: <noframes>", [] {
        auto res = parse("<head></head><frameset><noframes>", {});
        dom::Element expected{
                "html",
                {},
                {
                        dom::Element{"head"},
                        dom::Element{"frameset", {}, {dom::Element{"noframes"}}},
                },
        };
        expect_eq(res.document.html(), expected);
    });

    etest::test("InFrameset: </frameset>", [] {
        auto res = parse("<head></head><frameset></frameset>", {});
        dom::Element expected{
                "html",
                {},
                {
                        dom::Element{"head"},
                        dom::Element{"frameset"},
                },
        };
        expect_eq(res.document.html(), expected);
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
    in_body_tests();
    in_frameset_tests();
    return etest::run_all_tests();
}
