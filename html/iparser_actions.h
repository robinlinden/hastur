// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_IPARSER_ACTIONS_H_
#define HTML_IPARSER_ACTIONS_H_

#include "html/parser_states.h"

#include "html2/tokenizer.h"

#include <span>
#include <string>
#include <string_view>

namespace html {

enum class QuirksMode {
    NoQuirks,
    Quirks,
    LimitedQuirks,
};

class IActions {
public:
    virtual ~IActions() = default;

    virtual void set_doctype_name(std::string) = 0;
    virtual void set_quirks_mode(QuirksMode) = 0;
    virtual bool scripting() const = 0;
    virtual void insert_element_for(html2::StartTagToken const &) = 0;
    virtual void pop_current_node() = 0;
    virtual std::string_view current_node_name() const = 0;
    virtual void merge_into_html_node(std::span<html2::Attribute const>) = 0;
    virtual void insert_character(html2::CharacterToken const &) = 0;
    virtual void set_tokenizer_state(html2::State) = 0;
    virtual void store_original_insertion_mode(InsertionMode) = 0;
    virtual InsertionMode original_insertion_mode() = 0;
};

} // namespace html

#endif
