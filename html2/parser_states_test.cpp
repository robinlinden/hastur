// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/parser_states.h"

#include "html2/token.h"
#include "html2/tokenizer.h"

#include "dom/dom.h"
#include "etest/etest2.h"
#include "html/parser_actions.h"

#include <array>
#include <cstddef>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

using NodeVec = std::vector<dom::Node>;

namespace {
struct ParseResult {
    dom::Document document{};
};

struct ParseOptions {
    html2::InsertionMode initial_insertion_mode;
    bool scripting{false};
};

// TODO(robinlinden): This is very awkward, but I'll make it better later, I promise.
ParseResult parse(std::string_view html, ParseOptions const &opts) {
    html2::Tokenizer tokenizer{html, [&](auto &, auto const &) {
                               }};

    ParseResult res{};
    html2::InsertionMode mode{opts.initial_insertion_mode};
    std::vector<dom::Element *> open_elements;
    std::function<void(dom::Element const &)> on_element_closed{};
    html::Actions actions{
            res.document,
            tokenizer,
            opts.scripting,
            // TODO(robinlinden): Update tests to be happy with comments.
            html::CommentMode::Discard,
            mode,
            open_elements,
            on_element_closed,
    };

    auto on_token = [&](html2::Tokenizer &, html2::Token const &token) {
        mode = std::visit([&](auto &v) { return v.process(actions, token); }, mode).value_or(mode);
    };

    tokenizer = html2::Tokenizer{html, std::move(on_token)};
    tokenizer.run();
    return res;
}

void initial_tests(etest::Suite &s) {
    s.add_test("Initial: whitespace before doctype", [](etest::IActions &a) {
        auto res = parse("    <!DOCTYPE html>", {});
        a.expect_eq(res.document.doctype, "html");
        res = parse("\t\n\r <!DOCTYPE bad>", {});
        a.expect_eq(res.document.doctype, "bad");
    });

    s.add_test("Initial: comment", [](etest::IActions &a) {
        auto res = parse("<!-- hello --><!DOCTYPE html>", {});
        a.expect_eq(res.document.doctype, "html");
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("Initial: doctype, sane", [](etest::IActions &a) {
        auto res = parse("<!DOCTYPE html>", {});
        a.expect_eq(res.document.doctype, "html");
        a.expect_eq(res.document.mode, dom::Document::Mode::NoQuirks);
    });

    s.add_test("Initial: doctype, sane-ish", [](etest::IActions &a) {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01">)", {});
        a.expect_eq(res.document.mode, dom::Document::Mode::NoQuirks);
    });

    s.add_test("Initial: doctype, also sane-ish", [](etest::IActions &a) {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "def" "abc">)", {});
        a.expect_eq(res.document.mode, dom::Document::Mode::NoQuirks);
    });

    s.add_test("Initial: doctype, quirky 0", [](etest::IActions &a) {
        auto res = parse("<!DOCTYPE is_this_the_abyss?>", {});
        a.expect_eq(res.document.doctype, "is_this_the_abyss?");
        a.expect_eq(res.document.mode, dom::Document::Mode::Quirks);
    });

    s.add_test("Initial: doctype, quirky 1", [](etest::IActions &a) {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 FRAMESET//">)", {});
        a.expect_eq(res.document.mode, dom::Document::Mode::Quirks);
    });

    s.add_test("Initial: doctype, quirky 2", [](etest::IActions &a) {
        auto res = parse("<!DOCTYPE html SYSTEM http://www.IBM.com/data/dtd/v11/ibmxhtml1-transitional.dtd>", {});
        a.expect_eq(res.document.mode, dom::Document::Mode::Quirks);
    });

    s.add_test("Initial: doctype, quirky 3", [](etest::IActions &a) {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "HTML">)", {});
        a.expect_eq(res.document.mode, dom::Document::Mode::Quirks);
    });

    s.add_test("Initial: doctype, quirky 4", [](etest::IActions &a) {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "-//sun microsystems corp.//dtd hotjava html// i love this">)", {});
        a.expect_eq(res.document.mode, dom::Document::Mode::Quirks);
    });

    s.add_test("Initial: doctype, quirky-ish 0", [](etest::IActions &a) {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "-//w3c//dtd xhtml 1.0 transitional//hello">)", {});
        a.expect_eq(res.document.mode, dom::Document::Mode::LimitedQuirks);
    });

    s.add_test("Initial: doctype, quirky-ish 1", [](etest::IActions &a) {
        auto res = parse(R"(<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 FRAMESET//" "">)", {});
        a.expect_eq(res.document.mode, dom::Document::Mode::LimitedQuirks);
    });
}

void before_html_tests(etest::Suite &s) {
    s.add_test("BeforeHtml: doctype", [](etest::IActions &a) {
        auto res = parse("<!DOCTYPE html>", {.initial_insertion_mode = html2::BeforeHtml{}});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("BeforeHtml: comment", [](etest::IActions &a) {
        auto res = parse("<!DOCTYPE html><!-- hello --><html foo='bar'>", {});
        a.expect_eq(res.document.html(),
                dom::Element{"html", {{"foo", "bar"}}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("BeforeHtml: html tag", [](etest::IActions &a) {
        auto res = parse("<html foo='bar'>", {});
        a.expect_eq(res.document.html(),
                dom::Element{"html", {{"foo", "bar"}}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("BeforeHtml: boring whitespace before html is dropped", [](etest::IActions &a) {
        auto res = parse("<!DOCTYPE asdf>\t\n\f\r <html foo='bar'>", {});
        a.expect_eq(res.document.html(),
                dom::Element{"html", {{"foo", "bar"}}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("BeforeHtml: head end-tag", [](etest::IActions &a) {
        auto res = parse("</head>", {.initial_insertion_mode = html2::BeforeHtml{}});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("BeforeHtml: dropped end-tag", [](etest::IActions &a) {
        auto res = parse("</img>", {.initial_insertion_mode = html2::BeforeHtml{}});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });
}

void before_head_tests(etest::Suite &s) {
    s.add_test("BeforeHead: comment", [](etest::IActions &a) {
        auto res = parse("<html><!-- comment --><head foo='bar'>", {});
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {{"foo", "bar"}}}, dom::Element{"body"}}});
    });

    s.add_test("BeforeHead: doctype", [](etest::IActions &a) {
        auto res = parse("<html><!DOCTYPE html><head foo='bar'>", {});
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {{"foo", "bar"}}}, dom::Element{"body"}}});
    });

    s.add_test("BeforeHead: html tag", [](etest::IActions &a) {
        auto res = parse("<html foo=bar><html foo=baz hello=world>", {});
        auto const &head = std::get<dom::Element>(res.document.html().children.at(0));
        a.expect_eq(res.document.html().attributes, dom::AttrMap{{"foo", "bar"}, {"hello", "world"}});
        a.expect_eq(head, dom::Element{"head"});
    });

    s.add_test("BeforeHead: head tag", [](etest::IActions &a) {
        auto res = parse("<head foo='bar'>", {});
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {{"foo", "bar"}}}, dom::Element{"body"}}});
    });

    s.add_test("BeforeHead: end-tag fallthrough", [](etest::IActions &a) {
        auto res = parse("</head>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("BeforeHead: ignored end-tag", [](etest::IActions &a) {
        auto res = parse("</p><head foo=bar>", {});
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {{"foo", "bar"}}}, dom::Element{"body"}}});
    });

    s.add_test("BeforeHtml: boring whitespace before head is dropped", [](etest::IActions &a) {
        auto res = parse("<html>\t\n\f\r <head foo='bar'>", {});
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {{"foo", "bar"}}}, dom::Element{"body"}}});
    });
}

void in_head_tests(etest::Suite &s) {
    s.add_test("InHead: comment", [](etest::IActions &a) {
        auto res = parse("<html><head><!-- comment --><meta>", {});
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {dom::Element{"meta"}}}, dom::Element{"body"}}});
    });

    s.add_test("InHead: doctype", [](etest::IActions &a) {
        auto res = parse("<head><!doctype HTML>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("InHead: end tag parse error", [](etest::IActions &a) {
        auto res = parse("<head></p>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("InHead: html attributes are reparented", [](etest::IActions &a) {
        auto res = parse("<html foo=bar><head><html foo=baz hello=world>", {});
        auto const &head = std::get<dom::Element>(res.document.html().children.at(0));
        a.expect_eq(res.document.html().attributes, dom::AttrMap{{"foo", "bar"}, {"hello", "world"}});
        a.expect_eq(head, dom::Element{"head"});
    });

    s.add_test("InHead: base, basefont, bgsound, link", [](etest::IActions &a) {
        auto res = parse("<base> <basefont> <bgsound> <link>", {});

        auto head_children =
                NodeVec{dom::Element{"base"}, dom::Element{"basefont"}, dom::Element{"bgsound"}, dom::Element{"link"}};
        auto head = dom::Element{"head", {}, std::move(head_children)};

        a.expect_eq(res.document.html(), dom::Element{"html", {}, {std::move(head), dom::Element{"body"}}});
    });

    s.add_test("InHead: meta", [](etest::IActions &a) {
        auto res = parse("<meta>", {});
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {dom::Element{"meta"}}}, dom::Element{"body"}}});
    });

    s.add_test("InHead: title", [](etest::IActions &a) {
        auto res = parse("<title><body>&amp;</title>", {});
        auto title = dom::Element{"title", {}, {dom::Text{"<body>&"}}};
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(title)}}, dom::Element{"body"}}});
    });

    s.add_test("InHead: style", [](etest::IActions &a) {
        auto res = parse("<style>p { color: green; }</style>", {});
        auto style = dom::Element{"style", {}, {dom::Text{"p { color: green; }"}}};
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(style)}}, dom::Element{"body"}}});
    });

    s.add_test("InHead: style, abrupt eof", [](etest::IActions &a) {
        auto res = parse("<style>p { color: green; }", {});
        auto style = dom::Element{"style", {}, {dom::Text{"p { color: green; }"}}};
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(style)}}, dom::Element{"body"}}});
    });

    s.add_test("InHead: script", [](etest::IActions &a) {
        auto res = parse("<script>totally.js()</script>", {});
        auto script = dom::Element{"script", {}, {dom::Text{"totally.js()"}}};
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(script)}}, dom::Element{"body"}}});
    });

    s.add_test("InHead: head end tag", [](etest::IActions &a) {
        auto res = parse("</head>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("InHead: headhead", [](etest::IActions &a) {
        auto res = parse("<head><head>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("InHead: </template>", [](etest::IActions &a) {
        auto res = parse("<head></template>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });
}

void in_head_noscript_tests(etest::Suite &s) {
    s.add_test("InHeadNoscript: doctype is ignored", [](etest::IActions &a) {
        auto res = parse("<noscript><!doctype html></noscript>", {});
        auto const &html = res.document.html();
        a.expect_eq(html,
                dom::Element{"html", {}, {dom::Element{"head", {}, {dom::Element{"noscript"}}}, dom::Element{"body"}}});
    });

    s.add_test("InHeadNoscript: html attributes are reparented", [](etest::IActions &a) {
        auto res = parse("<html foo=bar><noscript><html foo=baz hello=world>", {});
        auto const &head = std::get<dom::Element>(res.document.html().children.at(0));
        a.expect_eq(res.document.html().attributes, dom::AttrMap{{"foo", "bar"}, {"hello", "world"}});
        a.expect_eq(head, dom::Element{"head", {}, {dom::Element{"noscript"}}});
    });

    s.add_test("InHeadNoScript: style", [](etest::IActions &a) {
        auto res = parse("<noscript><style>p { color: green; }", {});
        auto noscript = dom::Element{"noscript", {}, {dom::Element{"style", {}, {dom::Text{"p { color: green; }"}}}}};
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(noscript)}}, dom::Element{"body"}}});
    });

    s.add_test("InHeadNoScript: style w/ end tags", [](etest::IActions &a) {
        auto res = parse("<noscript><style>p { color: green; }</style></noscript>", {});
        auto noscript = dom::Element{"noscript", {}, {dom::Element{"style", {}, {dom::Text{"p { color: green; }"}}}}};
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(noscript)}}, dom::Element{"body"}}});
    });

    s.add_test("InHeadNoScript: br", [](etest::IActions &a) {
        auto res = parse("<noscript></br>", {});
        a.expect_eq(res.document.html(),
                dom::Element{
                        "html",
                        {},
                        {
                                dom::Element{"head", {}, {dom::Element{"noscript"}}},
                                dom::Element{"body", {}, {dom::Element{"br"}}},
                        },
                });
    });

    s.add_test("InHeadNoScript: noscript", [](etest::IActions &a) {
        auto res = parse("<noscript><noscript>", {});
        auto noscript = dom::Element{"noscript"};
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(noscript)}}, dom::Element{"body"}}});
    });
}

void after_head_tests(etest::Suite &s) {
    s.add_test("AfterHead: boring whitespace", [](etest::IActions &a) {
        auto res = parse("<head></head> ", {});
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head"}, dom::Text{" "}, dom::Element{"body"}}});
    });

    s.add_test("AfterHead: comment", [](etest::IActions &a) {
        auto res = parse("<head></head><!-- comment -->", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("AfterHead: doctype", [](etest::IActions &a) {
        auto res = parse("<head></head><!doctype html>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("AfterHead: html", [](etest::IActions &a) {
        auto res = parse("<html foo=bar><head></head><html foo=baz hello=world>", {});
        auto const &head = std::get<dom::Element>(res.document.html().children.at(0));
        a.expect_eq(res.document.html().attributes, dom::AttrMap{{"foo", "bar"}, {"hello", "world"}});
        a.expect_eq(head, dom::Element{"head"});
    });

    s.add_test("AfterHead: body", [](etest::IActions &a) {
        auto res = parse("<body>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("AfterHead: base, basefont, bgsound, link", [](etest::IActions &a) {
        auto res = parse("<head></head><base><basefont><bgsound><link>", {});

        auto head_children =
                NodeVec{dom::Element{"base"}, dom::Element{"basefont"}, dom::Element{"bgsound"}, dom::Element{"link"}};
        auto head = dom::Element{"head", {}, std::move(head_children)};

        a.expect_eq(res.document.html(), dom::Element{"html", {}, {std::move(head), dom::Element{"body"}}});
    });

    s.add_test("AfterHead: head", [](etest::IActions &a) {
        auto res = parse("<head></head><head>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("AfterHead: </template>", [](etest::IActions &a) {
        auto res = parse("<head></head></template>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("AfterHead: </body>", [](etest::IActions &a) {
        auto res = parse("<head></head></body>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("AfterHead: </html>", [](etest::IActions &a) {
        auto res = parse("<head></head></html>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("AfterHead: </br>", [](etest::IActions &a) {
        auto res = parse("<head></head></br>", {});
        a.expect_eq(res.document.html(),
                dom::Element{
                        "html",
                        {},
                        {dom::Element{"head"}, dom::Element{"body", {}, {dom::Element{"br"}}}},
                });
    });

    s.add_test("AfterHead: </error>", [](etest::IActions &a) {
        auto res = parse("<head></head></error>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("AfterHead: <frameset>", [](etest::IActions &a) {
        auto res = parse("<head></head><frameset>", {});
        a.expect_eq(res.document.html(), dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"frameset"}}});
    });

    s.add_test("AfterHead: <style>p { color: green; }", [](etest::IActions &a) {
        auto res = parse("<head></head><style>p { color: green; }</style>", {});
        auto style = dom::Element{"style", {}, {dom::Text{"p { color: green; }"}}};
        a.expect_eq(res.document.html(),
                dom::Element{"html", {}, {dom::Element{"head", {}, {std::move(style)}}, dom::Element{"body"}}});
    });
}

void in_body_tests(etest::Suite &s) {
    s.add_test("InBody: null character", [](etest::IActions &a) {
        auto res = parse("<body>\0"sv, {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(actual_body, dom::Element{"body"});
    });

    s.add_test("InBody: boring whitespace", [](etest::IActions &a) {
        auto res = parse("<body>\t"sv, {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(actual_body, dom::Element{"body", {}, {dom::Text{"\t"}}});
    });

    s.add_test("InBody: character", [](etest::IActions &a) {
        auto res = parse("<body>asdf"sv, {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(actual_body, dom::Element{"body", {}, {dom::Text{"asdf"}}});
    });

    s.add_test("InBody: comment", [](etest::IActions &a) {
        auto res = parse("<body><!-- comment -->", {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(actual_body, dom::Element{"body"});
    });

    s.add_test("InBody: doctype", [](etest::IActions &a) {
        auto res = parse("<body><!doctype html>", {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(actual_body, dom::Element{"body"});
    });

    s.add_test("InBody: in-head-element", [](etest::IActions &a) {
        auto res = parse("<body><title><html>&amp;</title>", {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(actual_body, dom::Element{"body", {}, {dom::Element{"title", {}, {dom::Text{"<html>&"}}}}});
    });

    s.add_test("InBody: <p> shielded by <button>", [](etest::IActions &a) {
        auto res = parse("<p><button><address>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        auto expected = dom::Element{
                "body",
                {},
                {dom::Element{"p", {}, {dom::Element{"button", {}, {dom::Element{"address"}}}}}},
        };

        a.expect_eq(body, expected);
    });

    s.add_test("InBody: <p> shielded by <marquee>", [](etest::IActions &a) {
        auto res = parse("<p><marquee><address>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        auto expected = dom::Element{
                "body",
                {},
                {dom::Element{"p", {}, {dom::Element{"marquee", {}, {dom::Element{"address"}}}}}},
        };

        a.expect_eq(body, expected);
    });

    s.add_test("InBody: template end tag", [](etest::IActions &a) {
        auto res = parse("<body></template>", {});
        auto const &actual_body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(actual_body, dom::Element{"body"});
    });

    s.add_test("InBody: automatically-closed p element", [](etest::IActions &a) {
        auto res = parse("<body><p>hello<p>world", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body,
                dom::Element{
                        "body",
                        {},
                        {
                                dom::Element{"p", {}, {dom::Text{"hello"}}},
                                dom::Element{"p", {}, {dom::Text{"world"}}},
                        },
                });
    });

    s.add_test("InBody: automatically-closed p element, not current element", [](etest::IActions &a) {
        auto res = parse("<body><p>hello<ruby><rb><p>world", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body,
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

    s.add_test("InBody: <hr>", [](etest::IActions &a) {
        auto res = parse("<body><p><hr>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"p"}, dom::Element{"hr"}}});
    });

    s.add_test("InBody: </br>", [](etest::IActions &a) {
        auto res = parse("<body></br>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"br"}}});
    });

    s.add_test("InBody: </ul>, no <ul>", [](etest::IActions &a) {
        auto res = parse("<body></ul>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body"});
    });

    s.add_test("InBody: </ul>, non-implicitly-closed node on stack", [](etest::IActions &a) {
        auto res = parse("<body><ul><a></ul>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"ul", {}, {dom::Element{"a"}}}}});
    });

    s.add_test("InBody: </li>, no <li>", [](etest::IActions &a) {
        auto res = parse("<body></li>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body"});
    });

    s.add_test("InBody: </li>, non-implicitly-closed node on stack", [](etest::IActions &a) {
        auto res = parse("<body><li><a></li>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"li", {}, {dom::Element{"a"}}}}});
    });

    s.add_test("InBody: <table>", [](etest::IActions &a) {
        auto res = parse("<body><table>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"table"}}});
    });

    s.add_test("InBody: <p><table>", [](etest::IActions &a) {
        auto res = parse("<body><p><table>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"p"}, dom::Element{"table"}}});
    });

    s.add_test("InBody: <p><table>, but quirky!", [](etest::IActions &a) {
        auto res = parse("<!DOCTYPE><body><p><table>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"p", {}, {dom::Element{"table"}}}}});
    });

    s.add_test("InBody: <template> doesn't crash", [](etest::IActions &) {
        std::ignore = parse("<body><template>", {}); //
    });

    for (auto tag : std::to_array<std::string_view>({"li", "dt", "dd"})) {
        s.add_test(std::format("InBody: <{}>", tag), [tag](etest::IActions &a) {
            auto html = std::format("<body><{}><p>hello<{}>world", tag, tag);
            auto res = parse(html, {});
            auto const &body = std::get<dom::Element>(res.document.html().children.at(1));

            a.expect_eq(body,
                    dom::Element{
                            "body",
                            {},
                            {
                                    dom::Element{std::string{tag}, {}, {dom::Element{"p", {}, {dom::Text{"hello"}}}}},
                                    dom::Element{std::string{tag}, {}, {dom::Text{"world"}}},
                            },
                    });
        });

        s.add_test(std::format("InBody: <{}>, weird specials", tag), [tag](etest::IActions &a) {
            auto res = parse(std::format("<body><section><p><{}>hello<a><{}>world", tag, tag), {});
            auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
            a.expect_eq(body.children.size(), std::size_t{1});
            auto const &section = std::get<dom::Element>(body.children.at(0));

            a.expect_eq(section,
                    dom::Element{
                            "section",
                            {},
                            {
                                    dom::Element{"p"},
                                    dom::Element{std::string{tag}, {}, {dom::Text{"hello"}, dom::Element{"a"}}},
                                    dom::Element{std::string{tag}, {}, {dom::Text{"world"}}},
                            },
                    });
        });
    }

    s.add_test("InBody: body end tag, disallowed element", [](etest::IActions &a) {
        auto res = parse("<body><foo></body>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"foo"}}});
    });

    s.add_test("InBody: body end tag, body not in scope", [](etest::IActions &a) {
        auto res = parse("<body><marquee></body>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"marquee"}}});
    });

    s.add_test("InBody: html end tag, disallowed element", [](etest::IActions &a) {
        auto res = parse("<body><foo></html>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"foo"}}});
    });

    s.add_test("InBody: html end tag, body not in scope", [](etest::IActions &a) {
        auto res = parse("<body><marquee></html>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"marquee"}}});
    });

    s.add_test("InBody: <noembed>", [](etest::IActions &a) {
        auto res = parse("<noembed>hello", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"noembed", {}, {dom::Text{"hello"}}}}});
    });
}

void in_table_tests(etest::Suite &s) {
    s.add_test("InTable: comment", [](etest::IActions &a) {
        auto res = parse("<table><!-- comment -->", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"table"}}});
    });

    s.add_test("InTable: doctype", [](etest::IActions &a) {
        auto res = parse("<table><!doctype html>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"table"}}});
    });

    s.add_test("InTable: </body>", [](etest::IActions &a) {
        // This will break once we implement more table parsing.
        auto res = parse("<table></html><tbody>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"table"}}});
    });

    s.add_test("InTable: <style>", [](etest::IActions &a) {
        auto res = parse("<table><style>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"table", {}, {dom::Element{"style"}}}}});
    });

    s.add_test("InTable: </table>", [](etest::IActions &a) {
        auto res = parse("<table></table>", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"table"}}});
    });
}

void in_table_text_tests(etest::Suite &s) {
    s.add_test("InTableText: hello", [](etest::IActions &a) {
        auto res = parse("<table>hello", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        auto const &table = std::get<dom::Element>(body.children.at(0));
        a.expect_eq(table, dom::Element{"table", {}, {dom::Text{"hello"}}});
    });

    s.add_test("InTableText: \0hello"s, [](etest::IActions &a) {
        auto res = parse("<table>\0hello"sv, {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        auto const &table = std::get<dom::Element>(body.children.at(0));
        a.expect_eq(table, dom::Element{"table", {}, {dom::Text{"hello"}}});
    });

    s.add_test("InTableText: boring whitespace", [](etest::IActions &a) {
        auto res = parse("<table>    ", {});
        auto const &body = std::get<dom::Element>(res.document.html().children.at(1));
        auto const &table = std::get<dom::Element>(body.children.at(0));
        a.expect_eq(table, dom::Element{"table", {}, {dom::Text{"    "}}});
    });
}

void in_frameset_tests(etest::Suite &s) {
    s.add_test("InFrameset: boring whitespace", [](etest::IActions &a) {
        auto res = parse("<head></head><frameset> ", {});
        dom::Element expected{
                "html",
                {},
                {
                        dom::Element{"head"},
                        dom::Element{"frameset", {}, {dom::Text{" "}}},
                },
        };
        a.expect_eq(res.document.html(), expected);
    });

    s.add_test("InFrameset: comment", [](etest::IActions &a) {
        auto res = parse("<head></head><frameset><!-- comment -->", {});
        dom::Element expected{
                "html",
                {},
                {dom::Element{"head"}, dom::Element{"frameset"}},
        };
        a.expect_eq(res.document.html(), expected);
    });

    s.add_test("InFrameset: doctype", [](etest::IActions &a) {
        auto res = parse("<head></head><frameset><!doctype html>", {});
        dom::Element expected{
                "html",
                {},
                {dom::Element{"head"}, dom::Element{"frameset"}},
        };
        a.expect_eq(res.document.html(), expected);
    });

    s.add_test("InFrameset: <html>", [](etest::IActions &a) {
        auto res = parse("<head></head><frameset><html foo=bar>", {});
        dom::Element expected{
                "html",
                {{"foo", "bar"}},
                {dom::Element{"head"}, dom::Element{"frameset"}},
        };
        a.expect_eq(res.document.html(), expected);
    });

    s.add_test("InFrameset: <frameset>", [](etest::IActions &a) {
        auto res = parse("<head></head><frameset><frameset>", {});
        dom::Element expected{
                "html",
                {},
                {
                        dom::Element{"head"},
                        dom::Element{"frameset", {}, {dom::Element{"frameset"}}},
                },
        };
        a.expect_eq(res.document.html(), expected);
    });

    s.add_test("InFrameset: <frame>", [](etest::IActions &a) {
        auto res = parse("<head></head><frameset><frame>", {});
        dom::Element expected{
                "html",
                {},
                {
                        dom::Element{"head"},
                        dom::Element{"frameset", {}, {dom::Element{"frame"}}},
                },
        };
        a.expect_eq(res.document.html(), expected);
    });

    s.add_test("InFrameset: <noframes>", [](etest::IActions &a) {
        auto res = parse("<head></head><frameset><noframes>", {});
        dom::Element expected{
                "html",
                {},
                {
                        dom::Element{"head"},
                        dom::Element{"frameset", {}, {dom::Element{"noframes"}}},
                },
        };
        a.expect_eq(res.document.html(), expected);
    });

    s.add_test("InFrameset: </frameset>", [](etest::IActions &a) {
        auto res = parse("<head></head><frameset></frameset>", {});
        dom::Element expected{
                "html",
                {},
                {
                        dom::Element{"head"},
                        dom::Element{"frameset"},
                },
        };
        a.expect_eq(res.document.html(), expected);
    });
}

} // namespace

int main() {
    etest::Suite s;
    initial_tests(s);
    before_html_tests(s);
    before_head_tests(s);
    in_head_tests(s);
    in_head_noscript_tests(s);
    after_head_tests(s);
    in_body_tests(s);
    in_table_tests(s);
    in_table_text_tests(s);
    in_frameset_tests(s);
    return s.run();
}
