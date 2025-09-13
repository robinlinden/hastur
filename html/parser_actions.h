// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSER_ACTIONS_H_
#define HTML_PARSER_ACTIONS_H_

#include "dom/dom.h"
#include "html2/iparser_actions.h"
#include "html2/parser_states.h"
#include "html2/token.h"
#include "html2/tokenizer.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace html {

enum class CommentMode : std::uint8_t {
    Keep,
    Discard,
};

class Actions : public html2::IActions {
public:
    Actions(dom::Document &document,
            html2::Tokenizer &tokenizer,
            bool scripting,
            CommentMode comment_mode,
            html2::InsertionMode &current_insertion_mode,
            std::vector<dom::Element *> &open_elements,
            std::function<void(dom::Element const &)> const &on_element_closed)
        : document_{document}, tokenizer_{tokenizer}, scripting_{scripting}, comment_mode_{comment_mode},
          current_insertion_mode_{current_insertion_mode}, open_elements_{open_elements},
          on_element_closed_{on_element_closed} {}

    void set_doctype_from(html2::DoctypeToken const &dt) override {
        document_.doctype = dt.name.value_or("");
        document_.public_identifier = dt.public_identifier.value_or("");
        document_.system_identifier = dt.system_identifier.value_or("");
    }

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

    html2::QuirksMode quirks_mode() const override {
        switch (document_.mode) {
            case dom::Document::Mode::NoQuirks:
                return html2::QuirksMode::NoQuirks;
            case dom::Document::Mode::Quirks:
                return html2::QuirksMode::Quirks;
            case dom::Document::Mode::LimitedQuirks:
                return html2::QuirksMode::LimitedQuirks;
        }
        return html2::QuirksMode::LimitedQuirks;
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

    void insert_element_for(html2::CommentToken const &token) override {
        if (comment_mode_ == CommentMode::Discard) {
            return;
        }

        if (open_elements_.empty()) {
            assert(std::get<dom::Element>(document_.html_node).children.empty());
            document_.pre_html_node_comments.push_back(dom::Comment{token.data});
            return;
        }

        auto &current_element = open_elements_.back();
        current_element->children.emplace_back(dom::Comment{token.data});
    }

    void pop_current_node() override {
        auto const *current_element = open_elements_.back();
        open_elements_.pop_back();

        // This may not be perfect as some elements can be opened and closed
        // multiple times (e.g. the head element), but it's good enough for now.
        if (on_element_closed_) {
            on_element_closed_(*current_element);
        }
    }

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

    void push_head_as_current_open_element() override {
        auto head = std::ranges::find_if(document_.html().children, [](auto const &node) {
            return std::holds_alternative<dom::Element>(node) && std::get<dom::Element>(node).name == "head";
        });

        assert(head != document_.html().children.end());
        assert(!std::ranges::contains(open_elements_, &std::get<dom::Element>(*head)));

        open_elements_.push_back(&std::get<dom::Element>(*head));
    }

    void remove_from_open_elements(std::string_view element_name) override {
        auto const it = std::ranges::find_if(open_elements_, [element_name](auto const &element) {
            return element->name == element_name; //
        });

        assert(it != open_elements_.end());
        open_elements_.erase(it);
    }

    void reconstruct_active_formatting_elements() override {
        // TODO(robinlinden): Implement.
    }

    std::vector<std::string_view> names_of_open_elements() const override {
        std::vector<std::string_view> names;
        names.reserve(open_elements_.size());
        std::ranges::transform(open_elements_, std::back_inserter(names), &dom::Element::name);
        std::ranges::reverse(names);
        return names;
    }

    void set_foster_parenting(bool) override {
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
    CommentMode comment_mode_;
    html2::InsertionMode original_insertion_mode_;
    html2::InsertionMode &current_insertion_mode_;
    std::vector<dom::Element *> &open_elements_;
    std::function<void(dom::Element const &)> const &on_element_closed_;
};

} // namespace html

#endif
