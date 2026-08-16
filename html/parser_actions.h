// SPDX-FileCopyrightText: 2023-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSER_ACTIONS_H_
#define HTML_PARSER_ACTIONS_H_

#include "html/iparser_actions.h"
#include "html/parser_states.h"
#include "html/token.h"
#include "html/tokenizer.h"

#include "dom/dom.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <optional>
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

class Actions : public IActions {
public:
    Actions(dom::Document &document,
            Tokenizer &tokenizer,
            bool scripting,
            CommentMode comment_mode,
            InsertionMode &current_insertion_mode,
            std::vector<dom::Element *> &open_elements,
            std::function<void(dom::Element const &)> const &on_element_closed)
        : document_{document}, tokenizer_{tokenizer}, scripting_{scripting}, comment_mode_{comment_mode},
          current_insertion_mode_{current_insertion_mode}, open_elements_{open_elements},
          on_element_closed_{on_element_closed} {}

    void set_doctype_from(DoctypeToken const &dt) override {
        document_.doctype = dt.name.value_or("");
        document_.public_identifier = dt.public_identifier.value_or("");
        document_.system_identifier = dt.system_identifier.value_or("");
    }

    void set_quirks_mode(QuirksMode mode) override {
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

    QuirksMode quirks_mode() const override {
        switch (document_.mode) {
            case dom::Document::Mode::NoQuirks:
                return QuirksMode::NoQuirks;
            case dom::Document::Mode::Quirks:
                return QuirksMode::Quirks;
            case dom::Document::Mode::LimitedQuirks:
                return QuirksMode::LimitedQuirks;
        }
        return QuirksMode::LimitedQuirks;
    }

    bool scripting() const override { return scripting_; }

    void insert_element_for(StartTagToken const &token) override {
        auto into_dom_attributes = [](std::vector<Attribute> const &attributes) -> dom::AttrMap {
            dom::AttrMap attrs{};
            for (auto const &[name, value] : attributes) {
                attrs[name] = value;
            }

            return attrs;
        };

        insert({token.tag_name, into_dom_attributes(token.attributes)});
    }

    void insert_element_for(CommentToken const &token) override {
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

    void merge_into_html_node(std::span<Attribute const> attrs) override {
        auto &html = document_.html();
        for (auto const &attr : attrs) {
            if (html.attributes.contains(attr.name)) {
                continue;
            }

            html.attributes[attr.name] = attr.value;
        }
    }

    void merge_into_body_node(std::span<Attribute const> attrs) override {
        auto it = std::ranges::find_if(document_.html().children, [](auto const &node) {
            return std::holds_alternative<dom::Element>(node) && std::get<dom::Element>(node).name == "body";
        });

        assert(it != document_.html().children.end());

        auto &body = std::get<dom::Element>(*it);
        for (auto const &attr : attrs) {
            if (body.attributes.contains(attr.name)) {
                continue;
            }

            body.attributes[attr.name] = attr.value;
        }
    }

    void insert_character(CharacterToken const &character) override {
        auto &current_element = open_elements_.back();
        if (current_element->children.empty() || !std::holds_alternative<dom::Text>(current_element->children.back())) {
            current_element->children.emplace_back(dom::Text{});
        }

        std::get<dom::Text>(current_element->children.back()).text += character.data;
    }

    void set_tokenizer_state(State state) override { tokenizer_.set_state(state); }

    void store_original_insertion_mode(InsertionMode mode) override { original_insertion_mode_ = std::move(mode); }

    InsertionMode original_insertion_mode() override { return std::move(original_insertion_mode_); }

    InsertionMode current_insertion_mode() const override { return current_insertion_mode_; }

    void set_frameset_ok(bool ok) override { is_frameset_ok_ = ok; }
    bool frameset_ok() const override { return is_frameset_ok_; }

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

    // TODO(robinlinden): This assumes that the element is both unique and in
    // scope. This is always true right now, but will it always be?
    void remove_from_its_parent_node(std::string_view element_name) override {
        while (current_node_name() != element_name) {
            pop_current_node();
            assert(!open_elements_.empty());
        }

        pop_current_node();
        assert(!open_elements_.empty());
        std::erase_if(open_elements_.back()->children, [element_name](auto const &child) {
            auto const *element = std::get_if<dom::Element>(&child);
            return element != nullptr && element->name == element_name;
        });
    }

    // https://html.spec.whatwg.org/#the-list-of-active-formatting-elements
    void reconstruct_active_formatting_elements() override {
        if (active_formatting_elements_.empty()) {
            return;
        }

        auto const &last = active_formatting_elements_.back();
        if (last.is_marker() || std::ranges::contains(open_elements_, last.element)) {
            return;
        }

        // Find the index to start creating elements from.
        std::size_t entry_index = active_formatting_elements_.size() - 1;

        while (true) {
            if (entry_index == 0) {
                break;
            }

            auto const &prev = active_formatting_elements_[entry_index - 1];
            if (!prev.is_marker() && !std::ranges::contains(open_elements_, prev.element)) {
                --entry_index;
                continue;
            }

            break;
        }

        // If we rewound to an entry that is neither a marker nor in the stack,
        // entry_index now points to the first entry that needs creating.
        for (std::size_t i = entry_index; i < active_formatting_elements_.size(); ++i) {
            auto &entry = active_formatting_elements_[i];
            assert(!entry.is_marker());
            insert(dom::Element{entry.name, entry.attributes});

            // The inserted element is now the current open element.
            entry.element = open_elements_.back();
        }
    }

    // https://html.spec.whatwg.org/#the-list-of-active-formatting-elements
    void push_current_element_onto_active_formatting_elements() override {
        assert(!open_elements_.empty());
        auto *el = open_elements_.back();
        ActiveFormattingElement afe{
                .element = el,
                .name = el->name,
                .attributes = el->attributes,
        };

        std::size_t start_index = 0;
        for (std::size_t i = active_formatting_elements_.size(); i-- > 0;) {
            if (active_formatting_elements_[i].is_marker()) {
                start_index = i + 1;
                break;
            }
        }

        int same_count = 0;
        for (std::size_t i = start_index; i < active_formatting_elements_.size(); ++i) {
            auto const &e = active_formatting_elements_[i];
            assert(!e.is_marker());
            if (e.name == afe.name && e.attributes == afe.attributes) {
                ++same_count;
            }
        }

        if (same_count >= 3) {
            for (std::size_t i = start_index; i < active_formatting_elements_.size(); ++i) {
                auto const &e = active_formatting_elements_[i];
                assert(!e.is_marker());
                if (e.name == afe.name && e.attributes == afe.attributes) {
                    active_formatting_elements_.erase(active_formatting_elements_.begin() + i);
                    break;
                }
            }
        }

        active_formatting_elements_.push_back(std::move(afe));
    }

    // https://html.spec.whatwg.org/#the-list-of-active-formatting-elements
    void push_formatting_marker() override { active_formatting_elements_.emplace_back(); }

    // https://html.spec.whatwg.org/#the-list-of-active-formatting-elements
    void clear_formatting_elements_up_to_last_marker() override {
        while (!active_formatting_elements_.empty()) {
            auto e = active_formatting_elements_.back();
            active_formatting_elements_.pop_back();
            if (e.is_marker()) {
                break;
            }
        }
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

    bool head_element_set() const override {
        return std::ranges::any_of(document_.html().children, [](auto const &node) {
            return std::holds_alternative<dom::Element>(node) && std::get<dom::Element>(node).name == "head";
        });
    }

    void set_fragment_parsing_context(std::string_view context) { fragment_parsing_context_ = context; }
    std::optional<std::string_view> fragment_parsing_context() const override { return fragment_parsing_context_; }

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
    Tokenizer &tokenizer_;
    bool scripting_;
    bool is_frameset_ok_{true};
    CommentMode comment_mode_;
    InsertionMode original_insertion_mode_;
    InsertionMode &current_insertion_mode_;
    std::vector<dom::Element *> &open_elements_;
    std::function<void(dom::Element const &)> const &on_element_closed_;
    std::optional<std::string_view> fragment_parsing_context_;

    struct ActiveFormattingElement {
        dom::Element *element{nullptr};
        std::string name;
        dom::AttrMap attributes;

        [[nodiscard]] constexpr bool is_marker() const { return element == nullptr; }
    };

    std::vector<ActiveFormattingElement> active_formatting_elements_;
};

} // namespace html

#endif
