// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML2_IPARSER_ACTIONS_H_
#define HTML2_IPARSER_ACTIONS_H_

#include "html2/parser_states.h"
#include "html2/token.h"
#include "html2/tokenizer.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string>
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
    virtual void set_frameset_ok(bool) = 0;
    virtual void push_head_as_current_open_element() = 0;
    virtual void remove_from_open_elements(std::string_view element_name) = 0;
    virtual void reconstruct_active_formatting_elements() = 0;

    // The most recently opened element is the first element in the list.
    virtual std::vector<std::string_view> names_of_open_elements() const = 0;

    virtual InsertionMode current_insertion_mode() const = 0;

    template<auto const &array>
    static constexpr bool is_in_array(std::string_view str) {
        return std::ranges::find(array, str) != std::cend(array);
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#has-an-element-in-scope
    bool has_element_in_scope(std::string_view element_name) const {
        static constexpr auto kScopeElements = std::to_array<std::string_view>({
                "applet", "caption", "html", "table", "td", "th", "marquee", "object", "template",
                // TODO(robinlinden): Add MathML and SVG elements.
                // MathML mi, MathML mo, MathML mn, MathML ms, MathML mtext,
                // MathML annotation-xml, SVG foreignObject, SVG desc, SVG
                // title,
        });

        for (auto const element : names_of_open_elements()) {
            if (is_in_array<kScopeElements>(element)) {
                return false;
            }

            if (element == element_name) {
                return true;
            }
        }

        return false;
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#has-an-element-in-button-scope
    bool has_element_in_button_scope(std::string_view element_name) const {
        for (auto const element : names_of_open_elements()) {
            if (element == "button") {
                return false;
            }

            if (element == element_name) {
                return true;
            }
        }

        return false;
    }
};

} // namespace html2

#endif
