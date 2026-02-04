// SPDX-FileCopyrightText: 2023-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/parser_states.h"

#include "html/iparser_actions.h"
#include "html/token.h"
#include "html/tokenizer.h"

#include "util/string.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace html {
namespace {

class InternalActions : public IActions {
public:
    explicit InternalActions(IActions &wrapped, InsertionMode mode_override)
        : wrapped_{wrapped}, current_insertion_mode_override_{std::move(mode_override)} {}

    void set_doctype_from(DoctypeToken const &dt) override { wrapped_.set_doctype_from(dt); }
    void set_quirks_mode(QuirksMode quirks) override { wrapped_.set_quirks_mode(quirks); }
    QuirksMode quirks_mode() const override { return wrapped_.quirks_mode(); }
    bool scripting() const override { return wrapped_.scripting(); }
    void insert_element_for(StartTagToken const &token) override { wrapped_.insert_element_for(token); }
    void insert_element_for(CommentToken const &token) override { wrapped_.insert_element_for(token); }
    void pop_current_node() override { wrapped_.pop_current_node(); }
    std::string_view current_node_name() const override { return wrapped_.current_node_name(); }
    void merge_into_html_node(std::span<Attribute const> attributes) override {
        wrapped_.merge_into_html_node(attributes);
    }
    void insert_character(CharacterToken const &token) override { wrapped_.insert_character(token); }
    void set_tokenizer_state(State state) override { wrapped_.set_tokenizer_state(state); }
    void store_original_insertion_mode(InsertionMode mode) override { wrapped_.store_original_insertion_mode(mode); }
    InsertionMode original_insertion_mode() override { return wrapped_.original_insertion_mode(); }
    InsertionMode current_insertion_mode() const override { return current_insertion_mode_override_; }
    void set_frameset_ok(bool ok) override { wrapped_.set_frameset_ok(ok); }
    void push_head_as_current_open_element() override { wrapped_.push_head_as_current_open_element(); }
    void remove_from_open_elements(std::string_view element_name) override {
        wrapped_.remove_from_open_elements(element_name);
    }
    void reconstruct_active_formatting_elements() override { wrapped_.reconstruct_active_formatting_elements(); }
    void push_current_element_onto_active_formatting_elements() override {
        wrapped_.push_current_element_onto_active_formatting_elements();
    }
    void push_formatting_marker() override { wrapped_.push_formatting_marker(); }
    void clear_formatting_elements_up_to_last_marker() override {
        wrapped_.clear_formatting_elements_up_to_last_marker();
    }
    std::vector<std::string_view> names_of_open_elements() const override { return wrapped_.names_of_open_elements(); }
    void set_foster_parenting(bool foster) override { wrapped_.set_foster_parenting(foster); }

private:
    IActions &wrapped_;
    InsertionMode current_insertion_mode_override_;
};

InternalActions current_insertion_mode_override(IActions &a, InsertionMode override) {
    return InternalActions{a, std::move(override)};
}

void pop_past(IActions &a, std::string_view element_name) {
    while (a.current_node_name() != element_name) {
        a.pop_current_node();
    }

    a.pop_current_node();
}

// A character token that is one of U+0009 CHARACTER TABULATION, U+000A LINE
// FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or U+0020
// SPACE.
bool is_boring_whitespace(Token const &token) {
    if (auto const &character = std::get_if<CharacterToken>(&token)) {
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
    if (std::ranges::contains(kQuirkyPublicIdentifiers, identifier)) {
        return true;
    }

    return std::ranges::any_of(
            kQuirkyStartsOfPublicIdentifier, [&](auto start) { return identifier.starts_with(start); });
}

constexpr bool is_quirky_when_system_identifier_is_empty(std::string_view public_identifier) {
    return public_identifier.starts_with("-//w3c//dtd html 4.01 frameset//")
            || public_identifier.starts_with("-//w3c//dtd html 4.01 transitional//");
}

[[nodiscard]] InsertionMode generic_raw_text_parse(IActions &a, StartTagToken const &token) {
    a.insert_element_for(token);
    a.set_tokenizer_state(State::Rawtext);
    a.store_original_insertion_mode(a.current_insertion_mode());
    return Text{};
}

[[nodiscard]] InsertionMode generic_rcdata_parse(IActions &a, StartTagToken const &token) {
    a.insert_element_for(token);
    a.set_tokenizer_state(State::Rcdata);
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

    return std::ranges::contains(kSpecial, node_name);
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

    return std::ranges::contains(kImplicityClosed, node_name);
}

void generate_implied_end_tags(IActions &a, std::optional<std::string_view> exception) {
    while (is_implicity_closed(a.current_node_name()) && a.current_node_name() != exception) {
        a.pop_current_node();
    }
}

// https://html.spec.whatwg.org/multipage/parsing.html#reset-the-insertion-mode-appropriately
InsertionMode appropriate_insertion_mode(IActions &a) {
    auto open_elements = a.names_of_open_elements();
    for (auto node : open_elements) {
        // TODO(robinlinden): Lots of table nonsense.
        if (node == "table") {
            return InTable{};
        }

        // TODO(robinlinden): Template nonsense. :(

        if (node == "head") {
            return InHead{};
        }

        if (node == "body") {
            return InBody{};
        }

        if (node == "frameset") {
            return InFrameset{};
        }

        if (node == "html") {
            // TODO(robinlinden): head element pointer.
            return AfterHead{};
        }
    }

    return InBody{};
}

template<auto const &scope_elements>
bool has_element_in_scope_impl(IActions const &a, std::string_view element_name) {
    for (auto const element : a.names_of_open_elements()) {
        if (element == element_name) {
            return true;
        }

        if (std::ranges::contains(scope_elements, element)) {
            return false;
        }
    }

    return false;
}

// https://html.spec.whatwg.org/multipage/parsing.html#has-an-element-in-scope
bool has_element_in_scope(IActions const &a, std::string_view element_name) {
    static constexpr auto kScopeElements = std::to_array<std::string_view>({
            "applet",
            "caption",
            "html",
            "table",
            "td",
            "th",
            "marquee",
            "object",
            "template",
            // TODO(robinlinden): Add MathML and SVG elements.
            // MathML mi, MathML mo, MathML mn, MathML ms, MathML mtext,
            // MathML annotation-xml, SVG foreignObject, SVG desc, SVG
            // title,
    });

    return has_element_in_scope_impl<kScopeElements>(a, element_name);
}

// https://html.spec.whatwg.org/multipage/parsing.html#has-an-element-in-button-scope
bool has_element_in_button_scope(IActions const &a, std::string_view element_name) {
    static constexpr auto kScopeElements = std::to_array<std::string_view>({
            "button",
            "applet",
            "caption",
            "html",
            "table",
            "td",
            "th",
            "marquee",
            "object",
            "template",
            // TODO(robinlinden): Add MathML and SVG elements.
            // MathML mi, MathML mo, MathML mn, MathML ms, MathML mtext,
            // MathML annotation-xml, SVG foreignObject, SVG desc, SVG
            // title,
    });

    return has_element_in_scope_impl<kScopeElements>(a, element_name);
}

// https://html.spec.whatwg.org/multipage/parsing.html#has-an-element-in-list-item-scope
bool has_element_in_list_item_scope(IActions const &a, std::string_view element_name) {
    static constexpr auto kScopeElements = std::to_array<std::string_view>({
            "ol",
            "ul",
            "applet",
            "caption",
            "html",
            "table",
            "td",
            "th",
            "marquee",
            "object",
            "template",
            // TODO(robinlinden): Add MathML and SVG elements.
    });

    return has_element_in_scope_impl<kScopeElements>(a, element_name);
}

bool has_element_in_table_scope(IActions const &a, std::string_view element_name) {
    static constexpr auto kScopeElements = std::to_array<std::string_view>({"html", "table", "template"});
    return has_element_in_scope_impl<kScopeElements>(a, element_name);
}
} // namespace

// https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
// Incomplete.
std::optional<InsertionMode> Initial::process(IActions &a, Token const &token) {
    if (is_boring_whitespace(token)) {
        return {};
    }

    if (auto const *comment = std::get_if<CommentToken>(&token)) {
        a.insert_element_for(*comment);
        return {};
    }

    if (auto const *doctype = std::get_if<DoctypeToken>(&token)) {
        a.set_doctype_from(*doctype);

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
std::optional<InsertionMode> BeforeHtml::process(IActions &a, Token const &token) {
    if (std::holds_alternative<DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    if (std::holds_alternative<CommentToken>(token)) {
        // TODO(robinlinden): Insert as last child.
        return {};
    }

    if (is_boring_whitespace(token)) {
        return {};
    }

    if (auto const *start = std::get_if<StartTagToken>(&token); start != nullptr && start->tag_name == "html") {
        a.insert_element_for(*start);
        return BeforeHead{};
    }

    static constexpr auto kAcceptableEndTags = std::to_array<std::string_view>({"head", "body", "html", "br"});
    if (auto const *end = std::get_if<EndTagToken>(&token);
            end != nullptr && (std::ranges::contains(kAcceptableEndTags, end->tag_name))) {
        // Fall through to "anything else."
    } else if (end != nullptr) {
        // Parse error.
        return {};
    }

    a.insert_element_for(StartTagToken{.tag_name = "html"});
    auto mode_override = current_insertion_mode_override(a, BeforeHead{});
    return BeforeHead{}.process(mode_override, token).value_or(BeforeHead{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#the-before-head-insertion-mode
std::optional<InsertionMode> BeforeHead::process(IActions &a, Token const &token) {
    if (is_boring_whitespace(token)) {
        return {};
    }

    if (auto const *comment = std::get_if<CommentToken>(&token)) {
        a.insert_element_for(*comment);
        return {};
    }

    if (std::holds_alternative<DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    if (auto const *start = std::get_if<StartTagToken>(&token)) {
        if (start->tag_name == "html") {
            return InBody{}.process(a, token);
        }

        if (start->tag_name == "head") {
            a.insert_element_for(*start);
            return InHead{};
        }
    } else if (auto const *end = std::get_if<EndTagToken>(&token)) {
        static constexpr auto kSortOfHandledEndTags = std::to_array<std::string_view>({"head", "body", "html", "br"});
        if (std::ranges::contains(kSortOfHandledEndTags, end->tag_name)) {
            // Treat as "anything else."
        } else {
            // Parse error.
            return {};
        }
    }

    a.insert_element_for(StartTagToken{.tag_name = "head"});
    auto mode_override = current_insertion_mode_override(a, InHead{});
    return InHead{}.process(mode_override, token).value_or(InHead{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead
// NOLINTNEXTLINE(misc-no-recursion)
std::optional<InsertionMode> InHead::process(IActions &a, Token const &token) {
    if (is_boring_whitespace(token)) {
        a.insert_character(std::get<CharacterToken>(token));
        return {};
    }

    if (auto const *comment = std::get_if<CommentToken>(&token)) {
        a.insert_element_for(*comment);
        return {};
    }

    if (std::holds_alternative<DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    auto const *start = std::get_if<StartTagToken>(&token);
    if (start != nullptr && start->tag_name == "html") {
        return InBody{}.process(a, token);
    }

    if (start != nullptr
            && (start->tag_name == "base" || start->tag_name == "basefont" || start->tag_name == "bgsound"
                    || start->tag_name == "link")) {
        a.insert_element_for(*start);
        a.pop_current_node();
        // TODO(robinlinden): Acknowledge the token's self-closing flag, if it is set.
        return {};
    }

    if (start != nullptr && start->tag_name == "meta") {
        a.insert_element_for(*start);
        a.pop_current_node();
        // TODO(robinlinden): Acknowledge the token's self-closing flag, if it is set.
        // TODO(robinlinden): Active speculative HTML parser nonsense.
        return {};
    }

    if (start != nullptr && start->tag_name == "title") {
        return generic_rcdata_parse(a, *start);
    }

    if (start != nullptr
            && ((start->tag_name == "noscript" && a.scripting()) || start->tag_name == "noframes"
                    || start->tag_name == "style")) {
        return generic_raw_text_parse(a, *start);
    }

    if (start != nullptr && start->tag_name == "noscript" && !a.scripting()) {
        a.insert_element_for(*start);
        return InHeadNoscript{};
    }

    if (start != nullptr && start->tag_name == "script") {
        // TODO(robinlinden): A lot of things. See spec.
        a.insert_element_for(*start);
        a.set_tokenizer_state(State::ScriptData);
        a.store_original_insertion_mode(a.current_insertion_mode());
        return Text{};
    }

    auto const *end = std::get_if<EndTagToken>(&token);
    if (end != nullptr && end->tag_name == "head") {
        assert(a.current_node_name() == "head");
        a.pop_current_node();
        return AfterHead{};
    }

    bool end_tag_as_anything_else = false;
    if (end != nullptr && (end->tag_name == "body" || end->tag_name == "html" || end->tag_name == "br")) {
        // Fall through to "anything else."
        end_tag_as_anything_else = true;
    }

    if (start != nullptr && start->tag_name == "template") {
        // TODO(robinlinden): Template nonsense.
        return {};
    }

    if (end != nullptr && end->tag_name == "template") {
        // TODO(robinlinden): Template nonsense.
        return {};
    }

    if ((start != nullptr && start->tag_name == "head") || (end != nullptr && !end_tag_as_anything_else)) {
        // Parse error.
        return {};
    }

    assert(a.current_node_name() == "head");
    a.pop_current_node();
    auto mode_override = current_insertion_mode_override(a, AfterHead{});
    return AfterHead{}.process(mode_override, token).value_or(AfterHead{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inheadnoscript
std::optional<InsertionMode> InHeadNoscript::process(IActions &a, Token const &token) {
    if (std::holds_alternative<DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    auto const *start = std::get_if<StartTagToken>(&token);
    if (start != nullptr && start->tag_name == "html") {
        return InBody{}.process(a, token);
    }

    auto const *end = std::get_if<EndTagToken>(&token);
    if (end != nullptr && end->tag_name == "noscript") {
        assert(a.current_node_name() == "noscript");
        a.pop_current_node();
        return InHead{};
    }

    static constexpr auto kInHeadElements =
            std::to_array<std::string_view>({"basefont", "bgsound", "link", "meta", "noframes", "style"});
    if ((start != nullptr && std::ranges::contains(kInHeadElements, start->tag_name))
            || std::holds_alternative<CommentToken>(token) || is_boring_whitespace(token)) {
        return InHead{}.process(a, token);
    }

    static constexpr auto kIgnoredStartTags = std::to_array<std::string_view>({"head", "noscript"});
    if (end != nullptr && end->tag_name == "br") {
        // Let the anything-else case handle this.
    } else if ((start != nullptr && std::ranges::contains(kIgnoredStartTags, start->tag_name)) || end != nullptr) {
        // Parse error, ignore the token.
        return {};
    }

    // Parse error.
    assert(a.current_node_name() == "noscript");
    a.pop_current_node();
    assert(a.current_node_name() == "head");
    auto mode_override = current_insertion_mode_override(a, InHead{});
    return InHead{}.process(mode_override, token).value_or(InHead{});
}

// https://html.spec.whatwg.org/multipage/parsing.html#the-after-head-insertion-mode
// NOLINTNEXTLINE(misc-no-recursion)
std::optional<InsertionMode> AfterHead::process(IActions &a, Token const &token) {
    if (is_boring_whitespace(token)) {
        a.insert_character(std::get<CharacterToken>(token));
        return {};
    }

    if (auto const *comment = std::get_if<CommentToken>(&token)) {
        a.insert_element_for(*comment);
        return {};
    }

    if (std::holds_alternative<DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    if (auto const *start = std::get_if<StartTagToken>(&token); start != nullptr) {
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
                "base",
                "basefont",
                "bgsound",
                "link",
                "meta",
                "noframes",
                "script",
                "style",
                "template",
                "title",
        });

        if (std::ranges::contains(kInHeadElements, start->tag_name)) {
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

    if (auto const *end = std::get_if<EndTagToken>(&token); end != nullptr) {
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
    auto mode_override = current_insertion_mode_override(a, InBody{});
    // TODO(robinlinden): Nicer fsm-management. We need to do it this way right
    // now in case the tag alters the state of the insertion mode, e.g. if we
    // get a <pre> tag before the <body> tag.
    auto in_body = InBody{};
    auto new_state = in_body.process(mode_override, token);
    return new_state.value_or(in_body);
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
// Incomplete.
// NOLINTNEXTLINE(misc-no-recursion)
std::optional<InsertionMode> InBody::process(IActions &a, Token const &token) {
    auto close_a_p_element = [&] {
        generate_implied_end_tags(a, "p");
        if (a.current_node_name() != "p") {
            // Parse error.
        }

        pop_past(a, "p");
    };

    auto const *character = std::get_if<CharacterToken>(&token);

    if (std::exchange(ignore_next_lf, false) && character != nullptr && character->data == '\n') {
        return {};
    }

    if (character != nullptr && character->data == '\0') {
        // Parse error.
        return {};
    }

    if (is_boring_whitespace(token)) {
        a.reconstruct_active_formatting_elements();
        a.insert_character(std::get<CharacterToken>(token));
        return {};
    }

    if (character != nullptr) {
        a.reconstruct_active_formatting_elements();
        a.insert_character(*character);
        a.set_frameset_ok(false);
        return {};
    }

    if (auto const *comment = std::get_if<CommentToken>(&token)) {
        a.insert_element_for(*comment);
        return {};
    }

    if (std::holds_alternative<DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    auto const *start = std::get_if<StartTagToken>(&token);
    if (start != nullptr && start->tag_name == "html") {
        // Parse error.
        // TODO(robinlinden): If there is a template element on the stack of open elements, then ignore the token.

        // The spec says to add attributes not already in the top element of the
        // stack of open elements. By top, they obviously mean the <html> tag.
        a.merge_into_html_node(start->attributes);
        return {};
    }

    static constexpr auto kInHeadElements = std::to_array<std::string_view>({
            "base",
            "basefont",
            "bgsound",
            "link",
            "meta",
            "noframes",
            "script",
            "style",
            "template",
            "title",
    });

    if (start != nullptr && std::ranges::contains(kInHeadElements, start->tag_name)) {
        return InHead{}.process(a, token);
    }

    auto const *end = std::get_if<EndTagToken>(&token);
    if (end != nullptr && end->tag_name == "template") {
        return InHead{}.process(a, token);
    }

    // TODO(robinlinden): Most things.

    if (end != nullptr && end->tag_name == "body") {
        if (!has_element_in_scope(a, "body")) {
            // Parse error.
            return {};
        }

        auto open_elements = a.names_of_open_elements();
        if (std::ranges::find_if_not(open_elements, [](auto const &name) {
                static constexpr auto kAllowedElements = std::to_array<std::string_view>({
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
                        "tbody",
                        "td",
                        "tfoot",
                        "th",
                        "thead",
                        "tr",
                        "body",
                        "html",
                });
                return std::ranges::contains(kAllowedElements, name);
            }) != std::cend(open_elements)) {
            // Parse error.
        }

        return AfterBody{};
    }

    if (end != nullptr && end->tag_name == "html") {
        if (!has_element_in_scope(a, "body")) {
            // Parse error.
            return {};
        }

        auto open_elements = a.names_of_open_elements();
        if (std::ranges::find_if_not(open_elements, [](auto const &name) {
                static constexpr auto kAllowedElements = std::to_array<std::string_view>({
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
                        "tbody",
                        "td",
                        "tfoot",
                        "th",
                        "thead",
                        "tr",
                        "body",
                        "html",
                });
                return std::ranges::contains(kAllowedElements, name);
            }) != std::cend(open_elements)) {
            // Parse error.
        }

        auto mode_override = current_insertion_mode_override(a, AfterBody{});
        return AfterBody{}.process(mode_override, token);
    }

    // TODO(robinlinden): Most things.

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
    if (start != nullptr && std::ranges::contains(kClosesPElements, start->tag_name)) {
        if (has_element_in_button_scope(a, "p")) {
            close_a_p_element();
        }

        a.insert_element_for(*start);
        return {};
    }

    static constexpr auto kHeadingTags = std::to_array<std::string_view>({
            "h1",
            "h2",
            "h3",
            "h4",
            "h5",
            "h6",
    });
    if (start != nullptr && std::ranges::contains(kHeadingTags, start->tag_name)) {
        if (has_element_in_button_scope(a, "p")) {
            close_a_p_element();
        }

        if (std::ranges::contains(kHeadingTags, a.current_node_name())) {
            // Parse error.
            a.pop_current_node();
        }

        a.insert_element_for(*start);
        return {};
    }

    static constexpr auto kPreLikeElements = std::to_array<std::string_view>({
            "pre",
            "listing",
    });
    if (start != nullptr && std::ranges::contains(kPreLikeElements, start->tag_name)) {
        if (has_element_in_button_scope(a, "p")) {
            close_a_p_element();
        }

        a.insert_element_for(*start);
        a.set_frameset_ok(false);
        ignore_next_lf = true;
        return {};
    }

    // TODO(robinlinden): Most things.

    if (start != nullptr && start->tag_name == "li") {
        a.set_frameset_ok(false);

        auto open_elements = a.names_of_open_elements();
        assert(!open_elements.empty());
        for (auto node : open_elements) {
            if (node == "li") {
                generate_implied_end_tags(a, "li");
                if (a.current_node_name() != "li") {
                    // Parse error.
                }

                pop_past(a, "li");
                break;
            }

            if (is_special(node) && node != "address" && node != "div" && node != "p") {
                break;
            }
        }

        if (has_element_in_button_scope(a, "p")) {
            close_a_p_element();
        }

        a.insert_element_for(*start);
        return {};
    }

    if (start != nullptr && (start->tag_name == "dd" || start->tag_name == "dt")) {
        a.set_frameset_ok(false);

        auto open_elements = a.names_of_open_elements();
        assert(!open_elements.empty());
        for (auto node : open_elements) {
            if (node == "dd" || node == "dt") {
                generate_implied_end_tags(a, node);
                if (a.current_node_name() != node) {
                    // Parse error.
                }

                pop_past(a, node);
                break;
            }

            if (is_special(node) && node != "address" && node != "div" && node != "p") {
                break;
            }
        }

        if (has_element_in_button_scope(a, "p")) {
            close_a_p_element();
        }

        a.insert_element_for(*start);
        return {};
    }

    // TODO(robinlinden): Most things.

    static constexpr auto kClosingTags = std::to_array<std::string_view>({
            "address",
            "article",
            "aside",
            "blockquote",
            "button",
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
            "listing",
            "main",
            "menu",
            "nav",
            "ol",
            "pre",
            "search",
            "section",
            "summary",
            "ul",
    });
    if (end != nullptr && std::ranges::contains(kClosingTags, end->tag_name)) {
        if (!has_element_in_scope(a, end->tag_name)) {
            // Parse error.
            return {};
        }

        generate_implied_end_tags(a, end->tag_name);
        if (a.current_node_name() != end->tag_name) {
            // Parse error.
        }

        pop_past(a, end->tag_name);
        return {};
    }

    // TODO(robinlinden): Most things.

    if (end != nullptr && end->tag_name == "li") {
        if (!has_element_in_list_item_scope(a, "li")) {
            // Parse error.
            return {};
        }

        generate_implied_end_tags(a, "li");
        if (a.current_node_name() != "li") {
            // Parse error.
        }

        pop_past(a, "li");
    }

    // TODO(robinlinden): Most things.

    static constexpr auto kFormattingElements = std::to_array<std::string_view>({
            "b",
            "big",
            "code",
            "em",
            "font",
            "i",
            "s",
            "small",
            "strike",
            "strong",
            "tt",
            "u",
    });
    if (start != nullptr && std::ranges::contains(kFormattingElements, start->tag_name)) {
        a.reconstruct_active_formatting_elements();
        a.insert_element_for(*start);
        a.push_current_element_onto_active_formatting_elements();
        return {};
    }

    // TODO(robinlinden): Most things.

    static constexpr auto kMarkerCreatingElements = std::to_array<std::string_view>({
            "applet",
            "marquee",
            "object",
    });
    if (start != nullptr && std::ranges::contains(kMarkerCreatingElements, start->tag_name)) {
        a.reconstruct_active_formatting_elements();
        a.insert_element_for(*start);
        a.push_formatting_marker();
        return {};
    }

    if (end != nullptr && std::ranges::contains(kMarkerCreatingElements, end->tag_name)) {
        if (!has_element_in_scope(a, end->tag_name)) {
            // Parse error.
            return {};
        }

        generate_implied_end_tags(a, std::nullopt);
        if (a.current_node_name() != end->tag_name) {
            // Parse error.
        }

        pop_past(a, end->tag_name);
        a.clear_formatting_elements_up_to_last_marker();
        return {};
    }

    if (start != nullptr && start->tag_name == "table") {
        if (a.quirks_mode() != QuirksMode::Quirks && has_element_in_button_scope(a, "p")) {
            close_a_p_element();
        }

        a.insert_element_for(*start);
        a.set_frameset_ok(false);
        return InTable{};
    }

    static constexpr auto kImmediatelyPoppedElements = std::to_array<std::string_view>({
            "area",
            "br",
            "embed",
            "img",
            "keygen",
            "wbr",
    });
    auto is_bad_br_end_tag = end != nullptr && end->tag_name == "br";
    if ((start != nullptr && std::ranges::contains(kImmediatelyPoppedElements, start->tag_name))
            || (is_bad_br_end_tag)) {
        if (is_bad_br_end_tag) {
            // Parse error.
            a.reconstruct_active_formatting_elements();
            a.insert_element_for(StartTagToken{.tag_name = "br"});
        } else {
            a.reconstruct_active_formatting_elements();
            a.insert_element_for(*start);
        }

        a.pop_current_node();
        // TODO(robinlinden): Acknowledge the token's self-closing flag, if it is set.
        a.set_frameset_ok(false);
        return {};
    }

    if (start != nullptr && start->tag_name == "input") {
        // TODO(robinlinden): Fragment-parsing case.

        if (has_element_in_scope(a, "select")) {
            // Parse error.
            pop_past(a, "select");
        }

        a.reconstruct_active_formatting_elements();
        a.insert_element_for(*start);
        a.pop_current_node();
        // TODO(robinlinden): Acknowledge the token's self-closing flag, if it is set.

        if (auto const type = std::ranges::find(start->attributes, "type", &Attribute::name);
                type == std::end(start->attributes) || util::lowercased(type->value) == "hidden") {
            a.set_frameset_ok(false);
        }

        return {};
    }

    static constexpr auto kSelfClosingElements = std::to_array<std::string_view>({
            "param",
            "source",
            "track",
    });
    if (start != nullptr && std::ranges::contains(kSelfClosingElements, start->tag_name)) {
        a.insert_element_for(*start);
        a.pop_current_node();
        // TODO(robinlinden): Acknowledge the token's self-closing flag, if it is set.
        return {};
    }

    // TODO(robinlinden): Most things.

    if (start != nullptr && start->tag_name == "hr") {
        if (has_element_in_button_scope(a, "p")) {
            close_a_p_element();
        }

        a.insert_element_for(*start);
        a.pop_current_node();
        // TODO(robinlinden): Acknowledge the token's self-closing flag, if it is set.
        a.set_frameset_ok(false);
        return {};
    }

    // TODO(robinlinden): Most things.

    if (start != nullptr && start->tag_name == "textarea") {
        a.insert_element_for(*start);

        // TODO(robinlinden): 2. If the next token is a U+000A LINE FEED (LF)
        // character token, then ignore that token and move on to the next one.
        // (Newlines at the start of textarea elements are ignored as an
        // authoring convenience.)

        a.set_tokenizer_state(State::Rcdata);
        a.store_original_insertion_mode(a.current_insertion_mode());
        a.set_frameset_ok(false);
        return Text{};
    }

    if (start != nullptr && start->tag_name == "xmp") {
        if (has_element_in_button_scope(a, "p")) {
            close_a_p_element();
        }

        a.reconstruct_active_formatting_elements();
        a.set_frameset_ok(false);
        return generic_raw_text_parse(a, *start);
    }

    if (start != nullptr && start->tag_name == "iframe") {
        a.set_frameset_ok(false);
        return generic_raw_text_parse(a, *start);
    }

    if (start != nullptr && ((start->tag_name == "noembed") || (start->tag_name == "noscript" && a.scripting()))) {
        return generic_raw_text_parse(a, *start);
    }

    // TODO(robinlinden): Most things.

    if (start != nullptr) {
        static constexpr auto kIgnoredStartTags = std::to_array<std::string_view>({
                "caption",
                "col",
                "colgroup",
                "frame",
                "head",
                "tbody",
                "td",
                "tfoot",
                "th",
                "thead",
                "tr",
        });

        if (std::ranges::contains(kIgnoredStartTags, start->tag_name)) {
            // Parse error.
            return {};
        }
    }

    if (start != nullptr) {
        a.insert_element_for(*start);
        a.reconstruct_active_formatting_elements();
        return {};
    }

    if (end != nullptr) {
        for (auto const name : a.names_of_open_elements()) {
            if (name == end->tag_name) {
                generate_implied_end_tags(a, end->tag_name);
                if (a.current_node_name() != end->tag_name) {
                    // Parse error.
                }

                pop_past(a, end->tag_name);
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
std::optional<InsertionMode> Text::process(IActions &a, Token const &token) {
    if (auto const *character = std::get_if<CharacterToken>(&token)) {
        assert(character->data != '\0');
        a.insert_character(*character);
        return {};
    }

    if (std::holds_alternative<EndOfFileToken>(token)) {
        // Parse error.
        // TODO(robinlinden): If current node is a script, set its already-started to true.
        a.pop_current_node();
        auto mode = a.original_insertion_mode();
        return std::visit([&](auto &m) { return m.process(a, token).value_or(m); }, mode);
    }

    if ([[maybe_unused]] auto const *end = std::get_if<EndTagToken>(&token)) {
        a.pop_current_node();
        return a.original_insertion_mode();
    }

    return {};
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-intable
// Incomplete.
std::optional<InsertionMode> InTable::process(IActions &a, Token const &token) {
    static constexpr auto kClearToTableContext = [](IActions &ac) {
        static constexpr auto kTableContextTags = std::to_array<std::string_view>({"table", "template", "html"});
        while (!std::ranges::contains(kTableContextTags, ac.current_node_name())) {
            ac.pop_current_node();
        }
    };

    auto const *character = std::get_if<CharacterToken>(&token);

    static constexpr auto kTableTextElements =
            std::to_array<std::string_view>({"table", "tbody", "template", "tfoot", "thead", "tr"});
    if (character != nullptr && std::ranges::contains(kTableTextElements, a.current_node_name())) {
        a.store_original_insertion_mode(a.current_insertion_mode());
        auto table_text = InTableText{};
        auto maybe_next = table_text.process(a, token);
        return maybe_next.value_or(std::move(table_text));
    }

    if (auto const *comment = std::get_if<CommentToken>(&token)) {
        a.insert_element_for(*comment);
        return {};
    }

    if (std::holds_alternative<DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    // TODO(robinlinden): Everything.

    auto const *start = std::get_if<StartTagToken>(&token);
    static constexpr auto kTableElements = std::to_array<std::string_view>({"td", "th", "tr"});
    if (start != nullptr && std::ranges::contains(kTableElements, start->tag_name)) {
        kClearToTableContext(a);
        a.insert_element_for(StartTagToken{.tag_name = "tbody"});
        return InTableBody{}.process(a, token).value_or(InTableBody{});
    }

    // TODO(robinlinden): Everything.

    auto const *end = std::get_if<EndTagToken>(&token);
    if (end != nullptr && end->tag_name == "table") {
        if (!has_element_in_table_scope(a, "table")) {
            // Parse error.
            return {};
        }

        pop_past(a, "table");
        return appropriate_insertion_mode(a);
    }

    static constexpr auto kBadEndTags = std::to_array<std::string_view>(
            {"body", "caption", "col", "colgroup", "html", "tbody", "td", "tfoot", "th", "thead", "tr"});
    if (end != nullptr && std::ranges::contains(kBadEndTags, end->tag_name)) {
        // Parse error.
        return {};
    }

    static constexpr auto kInHeadStartTags = std::to_array<std::string_view>({"style", "script", "template"});
    if ((start != nullptr && std::ranges::contains(kInHeadStartTags, start->tag_name))
            || (end != nullptr && end->tag_name == "template")) {
        auto mode_override = current_insertion_mode_override(a, InTable{});
        return InHead{}.process(mode_override, token);
    }

    // TODO(robinlinden): Everything.

    return {};
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-intabletext
std::optional<InsertionMode> InTableText::process(IActions &a, Token const &token) {
    if (auto const *character = std::get_if<CharacterToken>(&token); character != nullptr) {
        if (character->data == '\0') {
            // Parse error.
            return {};
        }

        pending_character_tokens.push_back(*character);
        return {};
    }

    if (std::ranges::any_of(pending_character_tokens, [](auto const &t) { return !is_boring_whitespace(t); })) {
        // Parse error.
        a.set_foster_parenting(true);
        for (auto const &pending : pending_character_tokens) {
            InBody{}.process(a, pending);
        }

        a.set_foster_parenting(false);
    } else {
        for (auto const &pending : pending_character_tokens) {
            a.insert_character(pending);
        }
    }

    auto mode = a.original_insertion_mode();
    return std::visit([&](auto &m) { return m.process(a, token).value_or(m); }, mode);
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-intbody
// Incomplete.
std::optional<InsertionMode> InTableBody::process(IActions &a, Token const &token) {
    static constexpr auto kClearToTableBodyContext = [](IActions &ac) {
        static constexpr auto kTableBodyContextTags =
                std::to_array<std::string_view>({"tbody", "tfoot", "thead", "template", "html"});
        while (!std::ranges::contains(kTableBodyContextTags, ac.current_node_name())) {
            ac.pop_current_node();
        }
    };

    // TODO(robinlinden): Everything.

    auto const *start = std::get_if<StartTagToken>(&token);
    static constexpr auto kInTableRowElements = std::to_array<std::string_view>({"th", "td"});
    if (start != nullptr && std::ranges::contains(kInTableRowElements, start->tag_name)) {
        // Parse error.
        kClearToTableBodyContext(a);
        a.insert_element_for(StartTagToken{.tag_name = "tr"});
        return InRow{}.process(a, token).value_or(InRow{});
    }

    // TODO(robinlinden): Everything.
    return {};
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-intr
// Incomplete.
std::optional<InsertionMode> InRow::process(IActions &a, Token const &token) {
    static constexpr auto kClearToTableRowContext = [](IActions &ac) {
        static constexpr auto kTableRowContextTags = std::to_array<std::string_view>({"tr", "template", "html"});
        while (!std::ranges::contains(kTableRowContextTags, ac.current_node_name())) {
            ac.pop_current_node();
        }
    };

    auto const *start = std::get_if<StartTagToken>(&token);
    if (start != nullptr && (start->tag_name == "td" || start->tag_name == "th")) {
        kClearToTableRowContext(a);
        a.insert_element_for(*start);
        a.push_formatting_marker();
        return InCell{};
    }

    // TODO(robinlinden): Everything.
    return {};
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-intd
std::optional<InsertionMode> InCell::process(IActions &a, Token const &token) {
    static constexpr auto kCloseCell = [](IActions &ac) {
        generate_implied_end_tags(ac, std::nullopt);
        if (ac.current_node_name() != "td" && ac.current_node_name() != "th") {
            // Parse error.
        }

        while (ac.current_node_name() != "td" && ac.current_node_name() != "th") {
            ac.pop_current_node();
        }

        ac.pop_current_node();
        ac.clear_formatting_elements_up_to_last_marker();
    };

    auto const *end = std::get_if<EndTagToken>(&token);
    if (end != nullptr && (end->tag_name == "td" || end->tag_name == "th")) {
        if (!has_element_in_table_scope(a, end->tag_name)) {
            // Parse error.
            return {};
        }

        generate_implied_end_tags(a, std::nullopt);
        if (a.current_node_name() != end->tag_name) {
            // Parse error.
        }

        pop_past(a, end->tag_name);
        a.clear_formatting_elements_up_to_last_marker();
        return InRow{};
    }

    auto const *start = std::get_if<StartTagToken>(&token);
    static constexpr auto kCellEndingStartTags = std::to_array<std::string_view>(
            {"caption", "col", "colgroup", "tbody", "td", "tfoot", "th", "thead", "tr"});
    if (start != nullptr && std::ranges::contains(kCellEndingStartTags, start->tag_name)) {
        assert(has_element_in_table_scope(a, "td") || has_element_in_table_scope(a, "th"));
        kCloseCell(a);
        auto override = current_insertion_mode_override(a, InRow{});
        return InRow{}.process(override, token).value_or(InRow{});
    }

    static constexpr auto kIgnoredEndTags =
            std::to_array<std::string_view>({"body", "caption", "col", "colgroup", "html"});
    if (end != nullptr && std::ranges::contains(kIgnoredEndTags, end->tag_name)) {
        // Parse error.
        return {};
    }

    static constexpr auto kTableRowEndTags =
            std::to_array<std::string_view>({"table", "tbody", "tfoot", "thead", "tr"});
    if (end != nullptr && std::ranges::contains(kTableRowEndTags, end->tag_name)) {
        if (!has_element_in_table_scope(a, end->tag_name)) {
            // Parse error.
            return {};
        }

        kCloseCell(a);
        auto override = current_insertion_mode_override(a, InRow{});
        return InRow{}.process(override, token).value_or(InRow{});
    }

    InBody{}.process(a, token);
    return {};
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-afterbody
// Incomplete.
std::optional<InsertionMode> AfterBody::process(IActions &, Token const &token) {
    auto const *end = std::get_if<EndTagToken>(&token);
    if (end != nullptr && end->tag_name == "html") {
        // TODO(robinlinden): Fragment-parsing stuff.
        return AfterAfterBody{};
    }

    return {};
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inframeset
std::optional<InsertionMode> InFrameset::process(IActions &a, Token const &token) {
    if (is_boring_whitespace(token)) {
        a.insert_character(std::get<CharacterToken>(token));
        return {};
    }

    if (auto const *comment = std::get_if<CommentToken>(&token)) {
        a.insert_element_for(*comment);
        return {};
    }

    if (std::holds_alternative<DoctypeToken>(token)) {
        // Parse error.
        return {};
    }

    if (auto const *start = std::get_if<StartTagToken>(&token); start != nullptr) {
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

    if (auto const *end = std::get_if<EndTagToken>(&token); end != nullptr && end->tag_name == "frameset") {
        // TODO(robinlinden): Fragment-parsing.
        a.pop_current_node();
        if (a.current_node_name() != "frameset") {
            return AfterFrameset{};
        }
    }

    if (std::holds_alternative<EndOfFileToken>(token)) {
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
std::optional<InsertionMode> AfterFrameset::process(IActions &, Token const &) {
    return {};
}

// https://html.spec.whatwg.org/multipage/parsing.html#the-after-after-body-insertion-mode
// Incomplete.
std::optional<InsertionMode> AfterAfterBody::process(IActions &a, Token const &token) {
    if (std::holds_alternative<EndOfFileToken>(token)) {
        return {};
    }

    auto mode_override = current_insertion_mode_override(a, InBody{});
    return InBody{}.process(mode_override, token).value_or(InBody{});
}

} // namespace html
