// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSER_STATES_H_
#define HTML_PARSER_STATES_H_

#include "dom/dom.h"
#include "html2/tokenizer.h"
#include "util/string.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <functional>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace html {

class Actions;

struct Initial;
struct BeforeHtml;
struct BeforeHead;
struct InHead;
struct InHeadNoscript;
struct AfterHead;
struct InBody;
struct Text;
struct InTable;
struct InTableText;
struct InCaption;
struct InColumnGroup;
struct InTableBody;
struct InRow;
struct InCell;
struct InSelect;
struct InSelectInTable;
struct InTemplate;
struct AfterBody;
struct InFrameset;
struct AfterFrameset;
struct AfterAfterBody;
struct AfterAfterFrameset;

// clang-format off
using InsertionMode = std::variant<Initial,
        BeforeHtml,
        BeforeHead,
        InHead,
        InHeadNoscript,
        AfterHead,
        // InBody,
        Text
#if 0
        InTable,
        InTableText,
        InCaption,
        InColumnGroup,
        InTableBody,
        InRow,
        InCell,
        InSelect,
        InSelectInTable,
        InTemplate,
        AfterBody,
        InFrameset,
        AfterFrameset,
        AfterAfterBody,
        AfterAfterFrameset
#endif
    >;
// clang-format on

struct InBody {};

struct InTable {};
struct InTableText {};
struct InCaption {};
struct InColumnGroup {};
struct InTableBody {};
struct InRow {};
struct InCell {};
struct InSelect {};
struct InSelectInTable {};
struct InTemplate {};
struct AfterBody {};
struct InFrameset {};
struct AfterFrameset {};
struct AfterAfterBody {};
struct AfterAfterFrameset {};

// https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
// Incomplete.
struct Initial {
    std::optional<InsertionMode> process(Actions &, html2::Token const &);
};

// https://html.spec.whatwg.org/multipage/parsing.html#the-before-html-insertion-mode
// Incomplete.
struct BeforeHtml {
    std::optional<InsertionMode> process(Actions &, html2::Token const &);
};

// https://html.spec.whatwg.org/multipage/parsing.html#the-before-head-insertion-mode
// Incomplete.
struct BeforeHead {
    std::optional<InsertionMode> process(Actions &, html2::Token const &);
};

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead
// Incomplete.
struct InHead {
    std::optional<InsertionMode> process(Actions &, html2::Token const &);
};

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inheadnoscript
// Incomplete.
struct InHeadNoscript {
    std::optional<InsertionMode> process(Actions &, html2::Token const &);
};

// https://html.spec.whatwg.org/multipage/parsing.html#the-after-head-insertion-mode
// Incomplete.
struct AfterHead {
    std::optional<InsertionMode> process(Actions &, html2::Token const &);
};

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-incdata
// Incomplete.
struct Text {
    std::optional<InsertionMode> process(Actions &, html2::Token const &);
};

class Actions {
public:
    Actions(dom::Document &document,
            html2::Tokenizer &tokenizer,
            bool scripting,
            std::stack<dom::Element *> &open_elements)
        : document_{document}, tokenizer_{tokenizer}, scripting_{scripting}, open_elements_{open_elements} {}

    dom::Document &document() { return document_; }
    std::stack<dom::Element *> &open_elements() { return open_elements_; }
    bool scripting() const { return scripting_; }

    void insert_element_for(html2::StartTagToken const &token) {
        auto into_dom_attributes = [](std::vector<html2::Attribute> const &attributes) -> dom::AttrMap {
            dom::AttrMap attrs{};
            for (auto const &[name, value] : attributes) {
                attrs[name] = value;
            }

            return attrs;
        };

        insert({token.tag_name, into_dom_attributes(token.attributes)});
    }

    void insert(dom::Element element) {
        dom::Node &node = open_elements_.top()->children.emplace_back(std::move(element));
        open_elements_.push(&std::get<dom::Element>(node));
    }

    void set_tokenizer_state(html2::State state) { tokenizer_.set_state(state); }

    void store_original_insertion_mode(InsertionMode mode) { original_insertion_mode_ = std::move(mode); }
    InsertionMode original_insertion_mode() { return std::move(original_insertion_mode_); }

private:
    dom::Document &document_;
    html2::Tokenizer &tokenizer_;
    bool scripting_;
    InsertionMode original_insertion_mode_;
    std::stack<dom::Element *> &open_elements_;
};

} // namespace html

#endif
