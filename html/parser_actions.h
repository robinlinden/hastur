// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSER_ACTIONS_H_
#define HTML_PARSER_ACTIONS_H_

#include "html/parser_states.h"

#include "dom/dom.h"
#include "html2/tokenizer.h"

#include <algorithm>
#include <stack>
#include <utility>
#include <variant>
#include <vector>

namespace html {

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
