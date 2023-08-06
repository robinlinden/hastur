// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/parser_states.h"

#include "html2/iparser_actions.h"
#include "html2/tokenizer.h"

#include "util/string.h"

#include <array>
#include <cassert>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

using namespace std::literals;

namespace html2 {
namespace {

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

// All public and system identifiers here are lowercased compared to the spec in
// order to simplify everything having to be done in a case-insensitive fashion.
constexpr auto kQuirkyPublicIdentifiers = std::to_array<std::string_view>({
        "-//w3o//dtd w3 html strict 3.0//en//",
        "-/w3c/dtd html 4.0 transitional/en",
        "html",
});
constexpr auto kQuirkyStartsOfPublicIdentifier = std::to_array<std::string_view>({
        "+//silmaril//dtd html pro v0r11 19970101//",
        "-//as//dtd html 3.0 aswedit + extensions//",
        "-//advasoft ltd//dtd html 3.0 aswedit + extensions//",
        "-//ietf//dtd html 2.0 level 1//",
        "-//ietf//dtd html 2.0 level 2//",
        "-//ietf//dtd html 2.0 strict level 1//",
        "-//ietf//dtd html 2.0 strict level 2//",
        "-//ietf//dtd html 2.0 strict//",
        "-//ietf//dtd html 2.0//",
        "-//ietf//dtd html 2.1e//",
        "-//ietf//dtd html 3.0//",
        "-//ietf//dtd html 3.2 final//",
        "-//ietf//dtd html 3.2//",
        "-//ietf//dtd html 3//",
        "-//ietf//dtd html level 0//",
        "-//ietf//dtd html level 1//",
        "-//ietf//dtd html level 2//",
        "-//ietf//dtd html level 3//",
        "-//ietf//dtd html strict level 0//",
        "-//ietf//dtd html strict level 1//",
        "-//ietf//dtd html strict level 2//",
        "-//ietf//dtd html strict level 3//",
        "-//ietf//dtd html strict//",
        "-//ietf//dtd html//",
        "-//metrius//dtd metrius presentational//",
        "-//microsoft//dtd internet explorer 2.0 html strict//",
        "-//microsoft//dtd internet explorer 2.0 html//",
        "-//microsoft//dtd internet explorer 2.0 tables//",
        "-//microsoft//dtd internet explorer 3.0 html strict//",
        "-//microsoft//dtd internet explorer 3.0 html//",
        "-//microsoft//dtd internet explorer 3.0 tables//",
        "-//netscape comm. corp.//dtd html//",
        "-//netscape comm. corp.//dtd strict html//",
        "-//o'reilly and associates//dtd html 2.0//",
        "-//o'reilly and associates//dtd html extended 1.0//",
        "-//o'reilly and associates//dtd html extended relaxed 1.0//",
        "-//sq//dtd html 2.0 hotmetal + extensions//",
        "-//softquad software//dtd hotmetal pro 6.0::19990601::extensions to html 4.0//",
        "-//softquad//dtd hotmetal pro 4.0::19971010::extensions to html 4.0//",
        "-//spyglass//dtd html 2.0 extended//",
        "-//sun microsystems corp.//dtd hotjava html//",
        "-//sun microsystems corp.//dtd hotjava strict html//",
        "-//w3c//dtd html 3 1995-03-24//",
        "-//w3c//dtd html 3.2 draft//",
        "-//w3c//dtd html 3.2 final//",
        "-//w3c//dtd html 3.2//",
        "-//w3c//dtd html 3.2s draft//",
        "-//w3c//dtd html 4.0 frameset//",
        "-//w3c//dtd html 4.0 transitional//",
        "-//w3c//dtd html experimental 19960712//",
        "-//w3c//dtd html experimental 970421//",
        "-//w3c//dtd w3 html//",
        "-//w3o//dtd w3 html 3.0//",
        "-//webtechs//dtd mozilla html 2.0//",
        "-//webtechs//dtd mozilla html//",
});
// https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
constexpr bool is_quirky_public_identifier(std::string_view identifier) {
    if (is_in_array<kQuirkyPublicIdentifiers>(identifier)) {
        return true;
    }

    for (auto start : kQuirkyStartsOfPublicIdentifier) {
        if (identifier.starts_with(start)) {
            return true;
        }
    }

    return false;
}
constexpr bool is_quirky_when_system_identifier_is_empty(std::string_view public_identifier) {
    return public_identifier.starts_with("-//w3c//dtd html 4.01 frameset//")
            || public_identifier.starts_with("-//w3c//dtd html 4.01 transitional//");
}

} // namespace

// https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
std::optional<InsertionMode> Initial::process(IActions &a, html2::Token const &token) {
    if (is_boring_whitespace(token)) {
        return {};
    }

    if (std::holds_alternative<html2::CommentToken>(token)) {
        // TODO(robinlinden): Insert as last child.
        return {};
    }

    if (auto const *doctype = std::get_if<html2::DoctypeToken>(&token)) {
        if (doctype->name) {
            a.set_doctype_name(*doctype->name);
        }

        using StringOverload = std::string (*)(std::string);
        auto const pub_id = doctype->public_identifier.transform(static_cast<StringOverload>(util::lowercased));
        auto const sys_id = doctype->system_identifier.transform(static_cast<StringOverload>(util::lowercased));
        auto const quirky_when_sys_id_is_empty =
                pub_id.transform(is_quirky_when_system_identifier_is_empty).value_or(false);
        if (doctype->force_quirks || doctype->name != "html"
                || pub_id.transform(is_quirky_public_identifier).value_or(false)
                || sys_id == "http://www.ibm.com/data/dtd/v11/ibmxhtml1-transitional.dtd"
                || (!sys_id.has_value() && quirky_when_sys_id_is_empty)) {
            a.set_quirks_mode(QuirksMode::Quirks);
        } else if (pub_id.has_value()
                && (pub_id->starts_with("-//w3c//dtd xhtml 1.0 frameset//")
                        || pub_id->starts_with("-//w3c//dtd xhtml 1.0 transitional//")
                        || (sys_id.has_value() && quirky_when_sys_id_is_empty))) {
            a.set_quirks_mode(QuirksMode::LimitedQuirks);
        }

        return BeforeHtml{};
    }

    return BeforeHtml{}.process(a, token).value_or(BeforeHtml{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#the-before-html-insertion-mode
std::optional<InsertionMode> BeforeHtml::process(IActions &a, html2::Token const &token) {
    if (std::holds_alternative<html2::CommentToken>(token)) {
        // TODO(robinlinden): Insert as last child.
        return {};
    }

    if (is_boring_whitespace(token)) {
        return {};
    }

    if (auto const *start = std::get_if<html2::StartTagToken>(&token); start != nullptr && start->tag_name == "html") {
        a.insert_element_for(*start);
        return BeforeHead{};
    }

    a.insert_element_for(html2::StartTagToken{.tag_name = "html"});
    return BeforeHead{}.process(a, token).value_or(BeforeHead{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#the-before-head-insertion-mode
std::optional<InsertionMode> BeforeHead::process(IActions &a, html2::Token const &token) {
    if (is_boring_whitespace(token)) {
        return {};
    }

    if (std::holds_alternative<html2::CommentToken>(token)) {
        // TODO(robinlinden): Insert a comment.
        return {};
    }

    if (auto const *start = std::get_if<html2::StartTagToken>(&token)) {
        if (start->tag_name == "head") {
            a.insert_element_for(*start);
            return InHead{};
        }
    }

    a.insert_element_for(html2::StartTagToken{.tag_name = "head"});
    return InHead{}.process(a, token).value_or(InHead{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead
std::optional<InsertionMode> InHead::process(IActions &a, html2::Token const &token) {
    if (is_boring_whitespace(token)) {
        // TODO(robinlinden): Should be inserting characters, but our last
        // parser didn't do that so it will require rewriting tests.
        return {};
    }

    if (std::holds_alternative<html2::CommentToken>(token)) {
        // TODO(robinlinden): Insert a comment.
        return {};
    }

    if (auto const *start = std::get_if<html2::StartTagToken>(&token)) {
        auto const &name = start->tag_name;
        // These branches won't be the same once we're more spec-complete.
        // NOLINTNEXTLINE(bugprone-branch-clone)
        if (name == "base" || name == "basefont" || name == "bgsound" || name == "link") {
            a.insert_element_for(*start);
            a.pop_current_node();
            return {};
        }

        if (name == "meta") {
            a.insert_element_for(*start);
            a.pop_current_node();
            return {};
        }

        if (name == "title") {
            a.insert_element_for(*start);
            a.set_tokenizer_state(html2::State::Rcdata);
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
            assert(a.current_node_name() == "head");
            a.pop_current_node();
            return AfterHead{};
        }
    }

    assert(a.current_node_name() == "head");
    a.pop_current_node();
    return AfterHead{}.process(a, token).value_or(AfterHead{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inheadnoscript
std::optional<InsertionMode> InHeadNoscript::process(IActions &a, html2::Token const &token) {
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
        assert(a.current_node_name() == "noscript");
        a.pop_current_node();
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
    assert(a.current_node_name() == "noscript");
    a.pop_current_node();
    assert(a.current_node_name() == "head");
    return InHead{}.process(a, token).value_or(InHead{});
}

std::optional<InsertionMode> AfterHead::process(IActions &, html2::Token const &) {
    return {};
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
std::optional<InsertionMode> InBody::process(IActions &a, html2::Token const &token) {
    if (auto const *start = std::get_if<html2::StartTagToken>(&token); start && start->tag_name == "html") {
        // Parse error.
        // TODO(robinlinden): If there is a template element on the stack of open elements, then ignore the token.

        // The spec says to add attributes not already in the top element of the
        // stack of open elements. By top, they obviously mean the <html> tag.
        a.merge_into_html_node(start->attributes);
    }

    return {};
}

std::optional<InsertionMode> Text::process(IActions &a, html2::Token const &token) {
    if (auto const *character = std::get_if<html2::CharacterToken>(&token)) {
        assert(character->data != '\0');
        a.insert_character(*character);
        return {};
    }

    if ([[maybe_unused]] auto const *end = std::get_if<html2::EndTagToken>(&token)) {
        a.pop_current_node();
        return a.original_insertion_mode();
    }

    return {};
}

} // namespace html2
