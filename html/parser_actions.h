// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSER_ACTIONS_H_
#define HTML_PARSER_ACTIONS_H_

#include "html/parser_states.h"

#include "dom/dom.h"
#include "html2/tokenizer.h"

#include <algorithm>
#include <cassert>
#include <span>
#include <stack>
#include <utility>
#include <variant>
#include <vector>

namespace html {

enum class QuirksMode {
    NoQuirks,
    Quirks,
    LimitedQuirks,
};

class Actions {
public:
    Actions(dom::Document &document,
            html2::Tokenizer &tokenizer,
            bool scripting,
            std::stack<dom::Element *> &open_elements)
        : document_{document}, tokenizer_{tokenizer}, scripting_{scripting}, open_elements_{open_elements} {}

    void set_doctype_name(std::string name) { document_.doctype = std::move(name); }

    void set_quirks_mode(QuirksMode mode) {
        document_.mode = [=] {
            switch (mode) {
                case QuirksMode::NoQuirks:
                    return dom::Document::Mode::NoQuirks;
                case QuirksMode::Quirks:
                    return dom::Document::Mode::Quirks;
                case QuirksMode::LimitedQuirks:
                    break;
            }
            return dom::Document::Mode::LimitedQuirks;
        }();
    }

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

    void pop_current_node() { open_elements_.pop(); }
    std::string_view current_node_name() const { return open_elements_.top()->name; }

    void merge_into_html_node(std::span<html2::Attribute const> attrs) {
        auto &html = document_.html();
        for (auto const &attr : attrs) {
            if (html.attributes.contains(attr.name)) {
                continue;
            }

            html.attributes[attr.name] = attr.value;
        }
    }

    void insert_character(html2::CharacterToken const &character) {
        auto &current_element = open_elements_.top();
        if (current_element->children.empty() || !std::holds_alternative<dom::Text>(current_element->children.back())) {
            current_element->children.emplace_back(dom::Text{});
        }

        std::get<dom::Text>(current_element->children.back()).text += character.data;
    }

    void set_tokenizer_state(html2::State state) { tokenizer_.set_state(state); }

    void store_original_insertion_mode(InsertionMode mode) { original_insertion_mode_ = std::move(mode); }
    InsertionMode original_insertion_mode() { return std::move(original_insertion_mode_); }

private:
    void insert(dom::Element element) {
        if (element.name == "html") {
            assert(open_elements_.empty());
            document_.html().name = std::move(element.name);
            document_.html().attributes = std::move(element.attributes);
            open_elements_.push(&document_.html());
            return;
        }

        dom::Node &node = open_elements_.top()->children.emplace_back(std::move(element));
        open_elements_.push(&std::get<dom::Element>(node));
    }

    dom::Document &document_;
    html2::Tokenizer &tokenizer_;
    bool scripting_;
    InsertionMode original_insertion_mode_;
    std::stack<dom::Element *> &open_elements_;
};

} // namespace html

#endif
