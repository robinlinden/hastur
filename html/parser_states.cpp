// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/parser_states.h"

#include "dom/dom.h"
#include "html2/tokenizer.h"

#include <array>
#include <cassert>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

using namespace std::literals;

namespace html {
namespace {
dom::AttrMap into_dom_attributes(std::vector<html2::Attribute> const &attributes) {
    dom::AttrMap attrs{};
    for (auto const &[name, value] : attributes) {
        attrs[name] = value;
    }

    return attrs;
}

// A character token that is one of U+0009 CHARACTER TABULATION, U+000A LINE
// FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or U+0020
// SPACE.
bool is_boring_whitespace(html2::Token const &token) {
    if (auto const &character = std::get_if<html2::CharacterToken>(&token)) {
        switch (character->data) {
            case '\t':
            case '\n':
            case '\f':
            case '\r':
            case ' ':
                return true;
            default:
                break;
        }
    }

    return false;
}

template<auto const &array>
constexpr bool is_in_array(std::string_view str) {
    return std::ranges::find(array, str) != std::cend(array);
}
} // namespace

// https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
std::optional<InsertionMode> Initial::process(Actions &a, html2::Token const &token) {
    if (is_boring_whitespace(token)) {
        return {};
    }

    if (auto const *doctype = std::get_if<html2::DoctypeToken>(&token)) {
        if (doctype->name) {
            a.document().doctype = *doctype->name;
        }

        return BeforeHtml{};
    }

    return BeforeHtml{}.process(a, token).value_or(BeforeHtml{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#the-before-html-insertion-mode
std::optional<InsertionMode> BeforeHtml::process(Actions &a, html2::Token const &token) {
    if (is_boring_whitespace(token)) {
        return {};
    }

    if (auto const *start = std::get_if<html2::StartTagToken>(&token); start != nullptr && start->tag_name == "html") {
        auto &html = a.document().html();
        html.name = start->tag_name;
        html.attributes = into_dom_attributes(start->attributes);
        a.open_elements().push(&html);
        return BeforeHead{};
    }

    auto &html = a.document().html();
    html.name = "html";
    a.open_elements().push(&html);
    return BeforeHead{}.process(a, token).value_or(BeforeHead{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#the-before-head-insertion-mode
std::optional<InsertionMode> BeforeHead::process(Actions &a, html2::Token const &token) {
    if (is_boring_whitespace(token)) {
        return {};
    }

    if (auto const *start = std::get_if<html2::StartTagToken>(&token)) {
        if (start->tag_name == "head") {
            a.insert_element_for(*start);
            return InHead{};
        }
    }

    a.insert({.name = "head"});
    return InHead{}.process(a, token).value_or(InHead{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead
std::optional<InsertionMode> InHead::process(Actions &a, html2::Token const &token) {
    if (is_boring_whitespace(token)) {
        // TODO(robinlinden): Should be inserting characters, but our last
        // parser didn't do that so it will require rewriting tests.
        return {};
    }

    if (auto const *start = std::get_if<html2::StartTagToken>(&token)) {
        auto const &name = start->tag_name;
        // These branches won't be the same once we're more spec-complete.
        // NOLINTNEXTLINE(bugprone-branch-clone)
        if (name == "base" || name == "basefont" || name == "bgsound" || name == "link") {
            a.insert_element_for(*start);
            a.open_elements().pop();
            return {};
        }

        if (name == "meta") {
            a.insert_element_for(*start);
            a.open_elements().pop();
            return {};
        }

        // These branches won't be the same once we've added support for rcdata in the tokenizer.
        // NOLINTNEXTLINE(bugprone-branch-clone)
        if (name == "title") {
            a.insert_element_for(*start);
            // TODO(robinlinden): Rcdata instead.
            a.set_tokenizer_state(html2::State::Rawtext);
            a.store_original_insertion_mode(InHead{});
            return Text{};
        }

        if (name == "noscript" && !a.scripting()) {
            a.insert_element_for(*start);
            return InHeadNoscript{};
        }

        if (name == "style") {
            a.insert_element_for(*start);
            a.set_tokenizer_state(html2::State::Rawtext);
            a.store_original_insertion_mode(InHead{});
            return Text{};
        }

        if (name == "script") {
            a.insert_element_for(*start);
            a.set_tokenizer_state(html2::State::ScriptData);
            a.store_original_insertion_mode(InHead{});
            return Text{};
        }
    } else if (auto const *end = std::get_if<html2::EndTagToken>(&token)) {
        if (end->tag_name == "head") {
            assert(a.open_elements().top()->name == "head");
            a.open_elements().pop();
            return AfterHead{};
        }
    }

    assert(a.open_elements().top()->name == "head");
    a.open_elements().pop();
    return AfterHead{}.process(a, token).value_or(AfterHead{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inheadnoscript
std::optional<InsertionMode> InHeadNoscript::process(Actions &a, html2::Token const &token) {
    if (std::holds_alternative<html2::DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    auto const *start = std::get_if<html2::StartTagToken>(&token);
    if (start && start->tag_name == "html") {
        return InBody{}.process(a, token);
    }

    auto const *end = std::get_if<html2::EndTagToken>(&token);
    if (end && end->tag_name == "noscript") {
        assert(a.open_elements().top()->name == "noscript");
        a.open_elements().pop();
        return InHead{};
    }

    static constexpr std::array kInHeadElements{"basefont"sv, "bgsound"sv, "link"sv, "meta"sv, "noframes"sv, "style"sv};
    if ((start && is_in_array<kInHeadElements>(start->tag_name)) || std::holds_alternative<html2::CommentToken>(token)
            || is_boring_whitespace(token)) {
        return InHead{}.process(a, token);
    }

    static constexpr std::array kIgnoredStartTags{"head"sv, "noscript"sv};
    if (end && end->tag_name == "br") {
        // Let the anything-else case handle this.
    } else if (start && is_in_array<kIgnoredStartTags>(start->tag_name)) {
        // Parse error, ignore the token.
        return {};
    }

    // Parse error.
    assert(a.open_elements().top()->name == "noscript");
    a.open_elements().pop();
    assert(a.open_elements().top()->name == "head");
    return InHead{}.process(a, token).value_or(InHead{});
}

std::optional<InsertionMode> AfterHead::process(Actions &, html2::Token const &) {
    return {};
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
std::optional<InsertionMode> InBody::process(Actions &a, html2::Token const &token) {
    if (auto const *start = std::get_if<html2::StartTagToken>(&token); start && start->tag_name == "html") {
        // Parse error.
        // TODO(robinlinden): If there is a template element on the stack of open elements, then ignore the token.
        auto &html = a.document().html();
        for (auto const &attr : start->attributes) {
            if (html.attributes.contains(attr.name)) {
                continue;
            }

            html.attributes[attr.name] = attr.value;
        }
    }

    return {};
}

std::optional<InsertionMode> Text::process(Actions &a, html2::Token const &token) {
    if (auto const *character = std::get_if<html2::CharacterToken>(&token)) {
        assert(character->data != '\0');

        auto &current_element = a.open_elements().top();
        if (current_element->children.empty() || !std::holds_alternative<dom::Text>(current_element->children.back())) {
            current_element->children.emplace_back(dom::Text{});
        }

        std::get<dom::Text>(current_element->children.back()).text += character->data;
        return {};
    }

    if ([[maybe_unused]] auto const *end = std::get_if<html2::EndTagToken>(&token)) {
        a.open_elements().pop();
        return a.original_insertion_mode();
    }

    return {};
}

} // namespace html
