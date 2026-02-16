// SPDX-FileCopyrightText: 2023-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_IPARSER_ACTIONS_H_
#define HTML_IPARSER_ACTIONS_H_

#include "html/parser_states.h"
#include "html/token.h"
#include "html/tokenizer.h"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace html {

enum class QuirksMode : std::uint8_t {
    NoQuirks,
    Quirks,
    LimitedQuirks,
};

class IActions {
public:
    virtual ~IActions() = default;

    virtual void set_doctype_from(DoctypeToken const &) = 0;
    virtual void set_quirks_mode(QuirksMode) = 0;
    virtual QuirksMode quirks_mode() const = 0;
    virtual bool scripting() const = 0;
    virtual void insert_element_for(StartTagToken const &) = 0;
    virtual void insert_element_for(CommentToken const &) = 0;
    virtual void pop_current_node() = 0;
    virtual std::string_view current_node_name() const = 0;
    virtual void merge_into_html_node(std::span<Attribute const>) = 0;
    virtual void merge_into_body_node(std::span<Attribute const>) = 0;
    virtual void insert_character(CharacterToken const &) = 0;
    virtual void set_tokenizer_state(State) = 0;
    virtual void store_original_insertion_mode(InsertionMode) = 0;
    virtual InsertionMode original_insertion_mode() = 0;
    virtual void set_frameset_ok(bool) = 0;
    virtual bool frameset_ok() const = 0;
    virtual void push_head_as_current_open_element() = 0;
    virtual void remove_from_open_elements(std::string_view element_name) = 0;
    virtual void remove_from_its_parent_node(std::string_view element_name) = 0;
    virtual void reconstruct_active_formatting_elements() = 0;
    virtual void push_current_element_onto_active_formatting_elements() = 0;
    virtual void push_formatting_marker() = 0;
    virtual void clear_formatting_elements_up_to_last_marker() = 0;
    virtual void set_foster_parenting(bool) = 0;

    // The most recently opened element is the first element in the list.
    // TODO(robinlinden): This is very unintuitive. The most recently opened element should be last.
    virtual std::vector<std::string_view> names_of_open_elements() const = 0;

    virtual InsertionMode current_insertion_mode() const = 0;
};

} // namespace html

#endif
