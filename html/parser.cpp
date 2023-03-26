// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/parser.h"

#include "util/string.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <string_view>
#include <variant>

using namespace std::literals;

namespace html {
namespace {

template<auto const &array>
constexpr bool is_in_array(std::string_view str) {
    return std::ranges::find(array, str) != std::cend(array);
}

dom::AttrMap into_dom_attributes(std::vector<html2::Attribute> const &attributes) {
    dom::AttrMap attrs{};
    for (auto const &[name, value] : attributes) {
        attrs[name] = value;
    }

    return attrs;
}

// https://developer.mozilla.org/en-US/docs/Glossary/Void_element
constexpr auto kImmediatelyPopped = std::to_array({"area"sv,
        "base"sv,
        "br"sv,
        "col"sv,
        "embed"sv,
        "hr"sv,
        "img"sv,
        "input"sv,
        "link"sv,
        "meta"sv,
        "param"sv,
        "source"sv,
        "track"sv,
        "wbr"sv});

constexpr std::array kAllowsParagraphEndTagOmission{
        "address"sv,
        "article"sv,
        "aside"sv,
        "blockquote"sv,
        "details"sv,
        "div"sv,
        "dl"sv,
        "fieldset"sv,
        "figcaption"sv,
        "figure"sv,
        "footer"sv,
        "form"sv,
        "h1"sv,
        "h2"sv,
        "h3"sv,
        "h4"sv,
        "h5"sv,
        "h6"sv,
        "header"sv,
        "hgroup"sv,
        "hr"sv,
        "main"sv,
        "menu"sv,
        "nav"sv,
        "ol"sv,
        "p"sv,
        "pre"sv,
        "section"sv,
        "table"sv,
        "ul"sv,
};

constexpr std::array kDisallowsParagraphEndTagOmissionWhenClosed{
        "a"sv,
        "audio"sv,
        "del"sv,
        "ins"sv,
        "map"sv,
        "noscript"sv,
        "video"sv,
};

} // namespace

void Parser::on_token(html2::Tokenizer &, html2::Token &&token) {
    // Everything in <head> and earlier is handled by the new parser.
    if (!std::holds_alternative<AfterHead>(insertion_mode_)) {
        insertion_mode_ = std::visit([&](auto &mode) { return mode.process(actions_, token); }, insertion_mode_)
                                  .value_or(insertion_mode_);
        if (auto const *end = std::get_if<html2::EndTagToken>(&token); end != nullptr && end->tag_name == "head") {
            return;
        }
    }

    if (std::holds_alternative<AfterHead>(insertion_mode_)) {
        std::visit(*this, token);
    }
}

void Parser::operator()(html2::DoctypeToken const &doctype) {
    if (doctype.name.has_value()) {
        doc_.doctype = *doctype.name;
    }
}

void Parser::operator()(html2::StartTagToken const &start_tag) {
    if (start_tag.tag_name == "script"sv) {
        tokenizer_.set_state(html2::State::ScriptData);
    }

    if (open_elements_.empty()) {
        spdlog::warn("Start tag [{}] encountered with no open elements", start_tag.tag_name);
        return;
    }

    // https://html.spec.whatwg.org/multipage/semantics.html#the-body-element
    // A body element's start tag can be omitted if the element is empty, or if
    // the first thing inside the body element is not ASCII whitespace or a
    // comment, except if the first thing inside the body element is a meta,
    // noscript, link, script, style, or template element.
    if (doc_.html().children.size() == 1 && start_tag.tag_name != "body") {
        auto &body = open_elements_.top()->children.emplace_back(dom::Element{.name{"body"}});
        open_elements_.push(&std::get<dom::Element>(body));
    }

    generate_text_node_if_needed();

    // https://html.spec.whatwg.org/multipage/grouping-content.html#the-p-element
    if (open_elements_.top()->name == "p" && is_in_array<kAllowsParagraphEndTagOmission>(start_tag.tag_name)) {
        open_elements_.pop();
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
    if (start_tag.tag_name == "noscript" && scripting_) {
        tokenizer_.set_state(html2::State::Rawtext);
    }

    auto &new_element = open_elements_.top()->children.emplace_back(
            dom::Element{start_tag.tag_name, into_dom_attributes(start_tag.attributes), {}});

    if (!start_tag.self_closing) {
        // This may seem risky since vectors will move their storage about
        // if they need it, but we only ever add new children to the
        // top-most element in the stack, so this pointer will be valid
        // until it's been popped from the stack and we add its siblings.
        open_elements_.push(std::get_if<dom::Element>(&new_element));
    }

    // Special cases from https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
    // Immediately popped off the stack of open elements special cases.
    if (!start_tag.self_closing && is_in_array<kImmediatelyPopped>(start_tag.tag_name)) {
        open_elements_.pop();
    }
}

void Parser::operator()(html2::EndTagToken const &end_tag) {
    if (open_elements_.empty()) {
        spdlog::warn("End tag [{}] encountered with no elements still open", end_tag.tag_name);
        return;
    }

    if (end_tag.tag_name == "html" && doc_.html().children.size() == 1) {
        if (open_elements_.top()->name == "html") {
            auto &body = open_elements_.top()->children.emplace_back(dom::Element{.name = "body"});
            open_elements_.push(&std::get<dom::Element>(body));
        }
    }

    generate_text_node_if_needed();

    // https://html.spec.whatwg.org/multipage/grouping-content.html#the-p-element
    // TODO(robinlinden): or if the parent element is an autonomous custom element.
    if (open_elements_.top()->name == "p" && end_tag.tag_name != "p"
            && !is_in_array<kDisallowsParagraphEndTagOmissionWhenClosed>(end_tag.tag_name)) {
        open_elements_.pop();
    }

    if (end_tag.tag_name == "html" && open_elements_.top()->name == "body") {
        open_elements_.pop();
    }

    auto const &expected_tag = open_elements_.top()->name;
    if (end_tag.tag_name != expected_tag) {
        spdlog::warn("Unexpected end_tag name, expected [{}] but got [{}]", expected_tag, end_tag.tag_name);
        return;
    }

    open_elements_.pop();
}

void Parser::operator()(html2::CommentToken const &) {
    // Do nothing.
}

void Parser::operator()(html2::CharacterToken const &character) {
    current_text_ << character.data;
}

void Parser::operator()(html2::EndOfFileToken const &) {
    if (!open_elements_.empty() && open_elements_.top()->name == "html" && open_elements_.top()->children.size() == 1) {
        auto &body = doc_.html().children.emplace_back(dom::Element{.name = "body"});
        open_elements_.push(&std::get<dom::Element>(body));
    }

    if (!open_elements_.empty()) {
        generate_text_node_if_needed();
    }

    if (!open_elements_.empty() && open_elements_.top()->name == "body") {
        open_elements_.pop();
    }

    // https://html.spec.whatwg.org/multipage/semantics.html#the-html-element
    if (!open_elements_.empty() && open_elements_.top()->name == "html") {
        open_elements_.pop();
    }

    if (!open_elements_.empty()) {
        spdlog::warn("EOF reached with [{}] elements still open", open_elements_.size());
    }
}

void Parser::generate_text_node_if_needed() {
    assert(!open_elements_.empty());
    auto text = std::exchange(current_text_, {}).str();
    bool is_uninteresting = std::ranges::all_of(text, [](char c) { return util::is_whitespace(c); });
    if (is_uninteresting) {
        return;
    }

    open_elements_.top()->children.emplace_back(dom::Text{std::move(text)});
}

} // namespace html
