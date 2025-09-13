// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML2_IPARSER_ACTIONS_H_
#define HTML2_IPARSER_ACTIONS_H_

#include "html2/parser_states.h"
#include "html2/token.h"
#include "html2/tokenizer.h"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace html2 {

enum class QuirksMode : std::uint8_t {
    NoQuirks,
    Quirks,
    LimitedQuirks,
};

class IActions {
public:
    virtual ~IActions() = default;

    virtual void set_doctype_from(html2::DoctypeToken const &) = 0;
    virtual void set_quirks_mode(QuirksMode) = 0;
    virtual QuirksMode quirks_mode() const = 0;
    virtual bool scripting() const = 0;
    virtual void insert_element_for(html2::StartTagToken const &) = 0;
    virtual void insert_element_for(html2::CommentToken const &) = 0;
    virtual void pop_current_node() = 0;
    virtual std::string_view current_node_name() const = 0;
    virtual void merge_into_html_node(std::span<html2::Attribute const>) = 0;
    virtual void insert_character(html2::CharacterToken const &) = 0;
    virtual void set_tokenizer_state(html2::State) = 0;
    virtual void store_original_insertion_mode(InsertionMode) = 0;
    virtual InsertionMode original_insertion_mode() = 0;
    virtual void set_frameset_ok(bool) = 0;
    virtual void push_head_as_current_open_element() = 0;
    virtual void remove_from_open_elements(std::string_view element_name) = 0;
    virtual void reconstruct_active_formatting_elements() = 0;
    virtual void set_foster_parenting(bool) = 0;

    // The most recently opened element is the first element in the list.
    virtual std::vector<std::string_view> names_of_open_elements() const = 0;

    virtual InsertionMode current_insertion_mode() const = 0;
};

} // namespace html2

#endif
