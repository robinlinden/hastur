// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSER_ACTIONS_H_
#define HTML_PARSER_ACTIONS_H_

#include "dom/dom.h"
#include "html2/iparser_actions.h"
#include "html2/parser_states.h"
#include "html2/tokenizer.h"

#include <algorithm>
#include <cassert>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace html {

class Actions : public html2::IActions {
public:
    Actions(dom::Document &document,
            html2::Tokenizer &tokenizer,
            bool scripting,
            html2::InsertionMode &current_insertion_mode,
            std::vector<dom::Element *> &open_elements)
        : document_{document}, tokenizer_{tokenizer}, scripting_{scripting},
          current_insertion_mode_{current_insertion_mode}, open_elements_{open_elements} {}

    void set_doctype_name(std::string name) override { document_.doctype = std::move(name); }

    void set_quirks_mode(html2::QuirksMode mode) override {
        document_.mode = [=] {
            switch (mode) {
                case html2::QuirksMode::NoQuirks:
                    return dom::Document::Mode::NoQuirks;
                case html2::QuirksMode::Quirks:
                    return dom::Document::Mode::Quirks;
                case html2::QuirksMode::LimitedQuirks:
                    break;
            }
            return dom::Document::Mode::LimitedQuirks;
        }();
    }

    bool scripting() const override { return scripting_; }

    void insert_element_for(html2::StartTagToken const &token) override {
        auto into_dom_attributes = [](std::vector<html2::Attribute> const &attributes) -> dom::AttrMap {
            dom::AttrMap attrs{};
            for (auto const &[name, value] : attributes) {
                attrs[name] = value;
            }

            return attrs;
        };

        insert({token.tag_name, into_dom_attributes(token.attributes)});
    }

    void pop_current_node() override { open_elements_.pop_back(); }
    std::string_view current_node_name() const override { return open_elements_.back()->name; }

    void merge_into_html_node(std::span<html2::Attribute const> attrs) override {
        auto &html = document_.html();
        for (auto const &attr : attrs) {
            if (html.attributes.contains(attr.name)) {
                continue;
            }

            html.attributes[attr.name] = attr.value;
        }
    }

    void insert_character(html2::CharacterToken const &character) override {
        auto &current_element = open_elements_.back();
        if (current_element->children.empty() || !std::holds_alternative<dom::Text>(current_element->children.back())) {
            current_element->children.emplace_back(dom::Text{});
        }

        std::get<dom::Text>(current_element->children.back()).text += character.data;
    }

    void set_tokenizer_state(html2::State state) override { tokenizer_.set_state(state); }

    void store_original_insertion_mode(html2::InsertionMode mode) override {
        original_insertion_mode_ = std::move(mode);
    }

    html2::InsertionMode original_insertion_mode() override { return std::move(original_insertion_mode_); }

    html2::InsertionMode current_insertion_mode() const override { return current_insertion_mode_; }

    void set_frameset_ok(bool) override {
        // TODO(robinlinden): Implement.
    }

private:
    void insert(dom::Element element) {
        if (element.name == "html") {
            assert(open_elements_.empty());
            document_.html().name = std::move(element.name);
            document_.html().attributes = std::move(element.attributes);
            open_elements_.push_back(&document_.html());
            return;
        }

        dom::Node &node = open_elements_.back()->children.emplace_back(std::move(element));
        open_elements_.push_back(&std::get<dom::Element>(node));
    }

    dom::Document &document_;
    html2::Tokenizer &tokenizer_;
    bool scripting_;
    html2::InsertionMode original_insertion_mode_;
    html2::InsertionMode &current_insertion_mode_;
    std::vector<dom::Element *> &open_elements_;
};

} // namespace html

#endif
