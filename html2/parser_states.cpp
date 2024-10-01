// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/parser_states.h"

#include "html2/iparser_actions.h"
#include "html2/token.h"
#include "html2/tokenizer.h"

#include "util/string.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

namespace html2 {
namespace {

class InternalActions : public IActions {
public:
    explicit InternalActions(IActions &wrapped, InsertionMode mode_override)
        : wrapped_{wrapped}, current_insertion_mode_override_{mode_override} {}

    void set_doctype_name(std::string name) override { wrapped_.set_doctype_name(std::move(name)); }
    void set_quirks_mode(QuirksMode quirks) override { wrapped_.set_quirks_mode(quirks); }
    bool scripting() const override { return wrapped_.scripting(); }
    void insert_element_for(html2::StartTagToken const &token) override { wrapped_.insert_element_for(token); }
    void pop_current_node() override { wrapped_.pop_current_node(); }
    std::string_view current_node_name() const override { return wrapped_.current_node_name(); }
    void merge_into_html_node(std::span<html2::Attribute const> attributes) override {
        wrapped_.merge_into_html_node(attributes);
    }
    void insert_character(html2::CharacterToken const &token) override { wrapped_.insert_character(token); }
    void set_tokenizer_state(html2::State state) override { wrapped_.set_tokenizer_state(state); }
    void store_original_insertion_mode(InsertionMode mode) override { wrapped_.store_original_insertion_mode(mode); }
    InsertionMode original_insertion_mode() override { return wrapped_.original_insertion_mode(); }
    InsertionMode current_insertion_mode() const override { return current_insertion_mode_override_; }
    void set_frameset_ok(bool ok) override { wrapped_.set_frameset_ok(ok); }
    void push_head_as_current_open_element() override { wrapped_.push_head_as_current_open_element(); }
    void remove_from_open_elements(std::string_view element_name) override {
        wrapped_.remove_from_open_elements(element_name);
    }
    void reconstruct_active_formatting_elements() override { wrapped_.reconstruct_active_formatting_elements(); }
    std::vector<std::string_view> names_of_open_elements() const override { return wrapped_.names_of_open_elements(); }

private:
    IActions &wrapped_;
    InsertionMode current_insertion_mode_override_;
};

InternalActions current_insertion_mode_override(IActions &a, InsertionMode override) {
    return InternalActions{a, override};
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

    return std::ranges::any_of(
            kQuirkyStartsOfPublicIdentifier, [&](auto start) { return identifier.starts_with(start); });
}

constexpr bool is_quirky_when_system_identifier_is_empty(std::string_view public_identifier) {
    return public_identifier.starts_with("-//w3c//dtd html 4.01 frameset//")
            || public_identifier.starts_with("-//w3c//dtd html 4.01 transitional//");
}

[[nodiscard]] InsertionMode generic_raw_text_parse(IActions &a, html2::StartTagToken const &token) {
    a.insert_element_for(token);
    a.set_tokenizer_state(html2::State::Rawtext);
    a.store_original_insertion_mode(a.current_insertion_mode());
    return Text{};
}

[[nodiscard]] InsertionMode generic_rcdata_parse(IActions &a, html2::StartTagToken const &token) {
    a.insert_element_for(token);
    a.set_tokenizer_state(html2::State::Rcdata);
    a.store_original_insertion_mode(a.current_insertion_mode());
    return Text{};
}

// https://html.spec.whatwg.org/multipage/parsing.html#special
bool is_special(std::string_view node_name) {
    static constexpr auto kSpecial = std::to_array<std::string_view>({
            "address",
            "applet",
            "area",
            "article",
            "aside",
            "base",
            "basefont",
            "bgsound",
            "blockquote",
            "body",
            "br",
            "button",
            "caption",
            "center",
            "col",
            "colgroup",
            "dd",
            "details",
            "dir",
            "div",
            "dl",
            "dt",
            "embed",
            "fieldset",
            "figcaption",
            "figure",
            "footer",
            "form",
            "frame",
            "frameset",
            "h1",
            "h2",
            "h3",
            "h4",
            "h5",
            "h6",
            "head",
            "header",
            "hgroup",
            "hr",
            "html",
            "iframe",
            "img",
            "input",
            "keygen",
            "li",
            "link",
            "listing",
            "main",
            "marquee",
            "menu",
            "meta",
            "nav",
            "noembed",
            "noframes",
            "noscript",
            "object",
            "ol",
            "p",
            "param",
            "plaintext",
            "pre",
            "script",
            "search",
            "section",
            "select",
            "source",
            "style",
            "summary",
            "table",
            "tbody",
            "td",
            "template",
            "textarea",
            "tfoot",
            "th",
            "thead",
            "title",
            "tr",
            "track",
            "ul",
            "wbr",
            "xmp",
    });

    return is_in_array<kSpecial>(node_name);
}

// https://html.spec.whatwg.org/multipage/parsing.html#closing-elements-that-have-implied-end-tags
bool is_implicity_closed(std::string_view node_name) {
    static constexpr auto kImplicityClosed = std::to_array<std::string_view>({
            "dd",
            "dt",
            "li",
            "optgroup",
            "option",
            "p",
            "rb",
            "rp",
            "rt",
            "rtc",
    });

    return is_in_array<kImplicityClosed>(node_name);
}

} // namespace

// https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
// Incomplete.
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

    auto mode_override = current_insertion_mode_override(a, BeforeHtml{});
    return BeforeHtml{}.process(mode_override, token).value_or(BeforeHtml{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#the-before-html-insertion-mode
std::optional<InsertionMode> BeforeHtml::process(IActions &a, html2::Token const &token) {
    if (std::holds_alternative<html2::DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

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

    static constexpr auto kAcceptableEndTags = std::to_array<std::string_view>({"head", "body", "html", "br"});
    if (auto const *end = std::get_if<html2::EndTagToken>(&token);
            end != nullptr && (is_in_array<kAcceptableEndTags>(end->tag_name))) {
        // Fall through to "anything else."
    } else if (end != nullptr) {
        // Parse error.
        return {};
    }

    a.insert_element_for(html2::StartTagToken{.tag_name = "html"});
    auto mode_override = current_insertion_mode_override(a, BeforeHead{});
    return BeforeHead{}.process(mode_override, token).value_or(BeforeHead{});
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

    if (std::holds_alternative<html2::DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    if (auto const *start = std::get_if<html2::StartTagToken>(&token)) {
        if (start->tag_name == "html") {
            return InBody{}.process(a, token);
        }

        if (start->tag_name == "head") {
            a.insert_element_for(*start);
            return InHead{};
        }
    } else if (auto const *end = std::get_if<html2::EndTagToken>(&token)) {
        static constexpr std::array kSortOfHandledEndTags{"head"sv, "body"sv, "html"sv, "br"sv};
        if (is_in_array<kSortOfHandledEndTags>(end->tag_name)) {
            // Treat as "anything else."
        } else {
            // Parse error.
            return {};
        }
    }

    a.insert_element_for(html2::StartTagToken{.tag_name = "head"});
    auto mode_override = current_insertion_mode_override(a, InHead{});
    return InHead{}.process(mode_override, token).value_or(InHead{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead
// Incomplete.
// NOLINTNEXTLINE(misc-no-recursion)
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

    if (std::holds_alternative<html2::DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    if (auto const *start = std::get_if<html2::StartTagToken>(&token)) {
        auto const &name = start->tag_name;
        if (name == "html") {
            return InBody{}.process(a, token);
        }

        if (name == "base" || name == "basefont" || name == "bgsound" || name == "link") {
            a.insert_element_for(*start);
            a.pop_current_node();
            // TODO(robinlinden): Acknowledge the token's self-closing flag, if it is set.
            return {};
        }

        if (name == "meta") {
            a.insert_element_for(*start);
            a.pop_current_node();
            // TODO(robinlinden): Acknowledge the token's self-closing flag, if it is set.
            // TODO(robinlinden): Active speculative HTML parser nonsense.
            return {};
        }

        if (name == "title") {
            return generic_rcdata_parse(a, *start);
        }

        if ((name == "noscript" && a.scripting()) || name == "noframes" || name == "style") {
            return generic_raw_text_parse(a, *start);
        }

        if (name == "noscript" && !a.scripting()) {
            a.insert_element_for(*start);
            return InHeadNoscript{};
        }

        if (name == "script") {
            // TODO(robinlinden): A lot of things. See spec.
            a.insert_element_for(*start);
            a.set_tokenizer_state(html2::State::ScriptData);
            a.store_original_insertion_mode(a.current_insertion_mode());
            return Text{};
        }

        if (name == "template") {
            // TODO(robinlinden): Template nonsense.
            return {};
        }
    } else if (auto const *end = std::get_if<html2::EndTagToken>(&token)) {
        if (end->tag_name == "head") {
            assert(a.current_node_name() == "head");
            a.pop_current_node();
            return AfterHead{};
        }

        if (end->tag_name == "body" || end->tag_name == "html" || end->tag_name == "br") {
            // Fall through to "anything else."
        } else {
            // Parse error.
            return {};
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
    if (start != nullptr && start->tag_name == "html") {
        return InBody{}.process(a, token);
    }

    auto const *end = std::get_if<html2::EndTagToken>(&token);
    if (end != nullptr && end->tag_name == "noscript") {
        assert(a.current_node_name() == "noscript");
        a.pop_current_node();
        return InHead{};
    }

    static constexpr std::array kInHeadElements{"basefont"sv, "bgsound"sv, "link"sv, "meta"sv, "noframes"sv, "style"sv};
    if ((start != nullptr && is_in_array<kInHeadElements>(start->tag_name))
            || std::holds_alternative<html2::CommentToken>(token) || is_boring_whitespace(token)) {
        return InHead{}.process(a, token);
    }

    static constexpr std::array kIgnoredStartTags{"head"sv, "noscript"sv};
    if (end != nullptr && end->tag_name == "br") {
        // Let the anything-else case handle this.
    } else if (start != nullptr && is_in_array<kIgnoredStartTags>(start->tag_name)) {
        // Parse error, ignore the token.
        return {};
    }

    // Parse error.
    assert(a.current_node_name() == "noscript");
    a.pop_current_node();
    assert(a.current_node_name() == "head");
    return InHead{}.process(a, token).value_or(InHead{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#the-after-head-insertion-mode
// NOLINTNEXTLINE(misc-no-recursion)
std::optional<InsertionMode> AfterHead::process(IActions &a, html2::Token const &token) {
    if (is_boring_whitespace(token)) {
        a.insert_character(std::get<html2::CharacterToken>(token));
        return {};
    }

    if (std::holds_alternative<html2::CommentToken>(token)) {
        // TODO(robinlinden): Insert.
        return {};
    }

    if (std::holds_alternative<html2::DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    if (auto const *start = std::get_if<html2::StartTagToken>(&token); start != nullptr) {
        if (start->tag_name == "html") {
            return InBody{}.process(a, token);
        }

        if (start->tag_name == "body") {
            a.insert_element_for(*start);
            a.set_frameset_ok(false);
            return InBody{};
        }

        if (start->tag_name == "frameset") {
            a.insert_element_for(*start);
            return InFrameset{};
        }

        static constexpr auto kInHeadElements = std::to_array<std::string_view>({
                "base"sv,
                "basefont"sv,
                "bgsound"sv,
                "link"sv,
                "meta"sv,
                "noframes"sv,
                "script"sv,
                "style"sv,
                "template"sv,
                "title"sv,
        });

        if (std::ranges::find(kInHeadElements, start->tag_name) != std::ranges::end(kInHeadElements)) {
            // Parse error.
            a.push_head_as_current_open_element();
            auto mode_override = current_insertion_mode_override(a, AfterHead{});
            auto new_state = InHead{}.process(mode_override, token);
            a.remove_from_open_elements("head");
            return new_state;
        }

        if (start->tag_name == "head") {
            // Parse error.
            return {};
        }
    }

    if (auto const *end = std::get_if<html2::EndTagToken>(&token); end != nullptr) {
        if (end->tag_name == "template") {
            // TODO(robinlinden): Process using InHead's rules once implemented.
            return {};
        }

        if (end->tag_name == "body" || end->tag_name == "html" || end->tag_name == "br") {
            // Treat as "anything else."
        } else {
            // Parse error.
            return {};
        }
    }

    a.insert_element_for({.tag_name = "body"});
    return InBody{}.process(a, token).value_or(InBody{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
// Incomplete.
// NOLINTNEXTLINE(misc-no-recursion)
std::optional<InsertionMode> InBody::process(IActions &a, html2::Token const &token) {
    auto close_a_p_element = [&] {
        while (a.current_node_name() != "p" && is_implicity_closed(a.current_node_name())) {
            a.pop_current_node();
        }

        if (a.current_node_name() != "p") {
            // Parse error.
        }

        while (a.current_node_name() != "p") {
            a.pop_current_node();
        }

        a.pop_current_node();
    };

    auto const *character = std::get_if<html2::CharacterToken>(&token);
    if (character != nullptr && character->data == '\0') {
        // Parse error.
        return {};
    }

    if (is_boring_whitespace(token)) {
        a.reconstruct_active_formatting_elements();
        a.insert_character(std::get<html2::CharacterToken>(token));
        return {};
    }

    if (character != nullptr) {
        a.reconstruct_active_formatting_elements();
        a.insert_character(*character);
        a.set_frameset_ok(false);
        return {};
    }

    if (std::holds_alternative<html2::CommentToken>(token)) {
        // TODO(robinlinden): Insert.
        return {};
    }

    if (std::holds_alternative<html2::DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    auto const *start = std::get_if<html2::StartTagToken>(&token);
    if (start != nullptr && start->tag_name == "html") {
        // Parse error.
        // TODO(robinlinden): If there is a template element on the stack of open elements, then ignore the token.

        // The spec says to add attributes not already in the top element of the
        // stack of open elements. By top, they obviously mean the <html> tag.
        a.merge_into_html_node(start->attributes);
        return {};
    }

    static constexpr auto kInHeadElements = std::to_array<std::string_view>({
            "base"sv,
            "basefont"sv,
            "bgsound"sv,
            "link"sv,
            "meta"sv,
            "noframes"sv,
            "script"sv,
            "style"sv,
            "template"sv,
            "title"sv,
    });

    if (start != nullptr && is_in_array<kInHeadElements>(start->tag_name)) {
        return InHead{}.process(a, token);
    }

    auto const *end = std::get_if<html2::EndTagToken>(&token);
    if (end != nullptr && end->tag_name == "template") {
        return InHead{}.process(a, token);
    }

    // TODO(robinlinden): Most things.

    // If the stack of open elements has a p element in button scope, then close a p element.

    // Insert an HTML element for the token.

    static constexpr auto kClosesPElements = std::to_array<std::string_view>({
            "address",
            "article",
            "aside",
            "blockquote",
            "center",
            "details",
            "dialog",
            "dir",
            "div",
            "dl",
            "fieldset",
            "figcaption",
            "figure",
            "footer",
            "header",
            "hgroup",
            "main",
            "menu",
            "nav",
            "ol",
            "p",
            "search",
            "section",
            "summary",
            "ul",
    });
    if (start != nullptr && is_in_array<kClosesPElements>(start->tag_name)) {
        if (a.has_element_in_button_scope("p")) {
            close_a_p_element();
        }

        a.insert_element_for(*start);
        return {};
    }

    // TODO(robinlinden): Most things.

    static constexpr auto kImmediatelyPoppedElements = std::to_array<std::string_view>({
            "area",
            "br",
            "embed",
            "img",
            "keygen",
            "wbr",
    });
    if (start != nullptr && is_in_array<kImmediatelyPoppedElements>(start->tag_name)) {
        a.reconstruct_active_formatting_elements();
        a.insert_element_for(*start);
        a.pop_current_node();
        // TODO(robinlinden): Acknowledge the token's self-closing flag, if it is set.
        a.set_frameset_ok(false);
        return {};
    }

    // TODO(robinlinden): Most things.

    if (start != nullptr && start->tag_name == "hr") {
        if (a.has_element_in_button_scope("p")) {
            close_a_p_element();
        }

        a.insert_element_for(*start);
        a.pop_current_node();
        // TODO(robinlinden): Acknowledge the token's self-closing flag, if it is set.
        a.set_frameset_ok(false);
        return {};
    }

    // TODO(robinlinden): Most things.

    if (start != nullptr && ((start->tag_name == "noembed") || (start->tag_name == "noscript" && a.scripting()))) {
        return generic_raw_text_parse(a, *start);
    }

    // TODO(robinlinden): Most things.

    if (start != nullptr) {
        a.insert_element_for(*start);
        a.reconstruct_active_formatting_elements();
        return {};
    }

    // TODO(robinlinden): Non-spec-compliant hack, remove.
    if (end != nullptr && (end->tag_name == "body" || end->tag_name == "html")) {
        return {};
    }

    if (end != nullptr) {
        for (auto const name : a.names_of_open_elements()) {
            if (name == end->tag_name) {
                // Generate implied end tags.
                while (a.current_node_name() != end->tag_name && is_implicity_closed(a.current_node_name())) {
                    a.pop_current_node();
                }

                if (a.current_node_name() != end->tag_name) {
                    // Parse error.
                }

                while (a.current_node_name() != end->tag_name) {
                    a.pop_current_node();
                }

                a.pop_current_node();
                break;
            }

            if (is_special(name)) {
                // Parse error.
                return {};
            }
        }

        return {};
    }

    return {};
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-incdata
// Incomplete.
std::optional<InsertionMode> Text::process(IActions &a, html2::Token const &token) {
    if (auto const *character = std::get_if<html2::CharacterToken>(&token)) {
        assert(character->data != '\0');
        a.insert_character(*character);
        return {};
    }

    if (std::holds_alternative<html2::EndOfFileToken>(token)) {
        // Parse error.
        // TODO(robinlinden): If current node is a script, set its already-started to true.
        a.pop_current_node();
        auto mode = a.original_insertion_mode();
        return std::visit([&](auto &m) { return m.process(a, token).value_or(m); }, mode);
    }

    if ([[maybe_unused]] auto const *end = std::get_if<html2::EndTagToken>(&token)) {
        a.pop_current_node();
        return a.original_insertion_mode();
    }

    return {};
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inframeset
std::optional<InsertionMode> InFrameset::process(IActions &a, html2::Token const &token) {
    if (is_boring_whitespace(token)) {
        a.insert_character(std::get<html2::CharacterToken>(token));
        return {};
    }

    if (std::holds_alternative<html2::CommentToken>(token)) {
        // TODO(robinlinden): Insert.
        return {};
    }

    if (std::holds_alternative<html2::DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    if (auto const *start = std::get_if<html2::StartTagToken>(&token); start != nullptr) {
        if (start->tag_name == "html") {
            return InBody{}.process(a, token);
        }

        if (start->tag_name == "frameset") {
            a.insert_element_for(*start);
            return {};
        }

        if (start->tag_name == "frame") {
            a.insert_element_for(*start);
            a.pop_current_node();
            // TODO(robinlinden): Acknowledge the token's self-closing flag, if it is set.
            return {};
        }

        if (start->tag_name == "noframes") {
            auto mode_override = current_insertion_mode_override(a, InFrameset{});
            return InHead{}.process(mode_override, token);
        }
    }

    if (auto const *end = std::get_if<html2::EndTagToken>(&token); end != nullptr && end->tag_name == "frameset") {
        // TODO(robinlinden): Fragment-parsing.
        a.pop_current_node();
        if (a.current_node_name() != "frameset") {
            return AfterFrameset{};
        }
    }

    if (std::holds_alternative<html2::EndOfFileToken>(token)) {
        if (a.current_node_name() != "html") {
            // Parse error.
        }

        return {};
    }

    // Parse error.
    return {};
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-afterframeset
// Incomplete.
std::optional<InsertionMode> AfterFrameset::process(IActions &, html2::Token const &) {
    return {};
}

} // namespace html2
