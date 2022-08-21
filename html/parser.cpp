// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
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
    return std::find(std::cbegin(array), std::cend(array), str) != std::cend(array);
}

dom::AttrMap into_dom_attributes(std::vector<html2::Attribute> const &attributes) {
    dom::AttrMap attrs{};
    for (auto const &[name, value] : attributes) {
        attrs[name] = value;
    }

    return attrs;
}

constexpr auto kImmediatelyPopped = std::to_array(
        {"br"sv, "hr"sv, "img"sv, "input"sv, "link"sv, "meta"sv, "param"sv, "source"sv, "track"sv, "wbr"sv});

} // namespace

void Parser::on_token(html2::Tokenizer &, html2::Token &&token) {
    std::visit(*this, token);
}

void Parser::operator()(html2::DoctypeToken const &doctype) {
    if (doctype.name.has_value()) {
        doc_.doctype = *doctype.name;
    }
}

void Parser::operator()(html2::StartTagToken const &start_tag) {
    if (start_tag.tag_name == "html"sv) {
        doc_.html().name = start_tag.tag_name;
        doc_.html().attributes = into_dom_attributes(start_tag.attributes);
        open_elements_.push(&doc_.html());
        seen_html_tag_ = true;
        return;
    }

    if (start_tag.tag_name == "script"sv) {
        tokenizer_.set_state(html2::State::ScriptData);
    }

    if (open_elements_.empty() && !seen_html_tag_) {
        spdlog::warn("Start tag [{}] encountered before html element was opened", start_tag.tag_name);
        doc_.html().name = "html"s;
        open_elements_.push(&doc_.html());
        seen_html_tag_ = true;
    } else if (open_elements_.empty()) {
        spdlog::warn("Start tag [{}] encountered with no open elements", start_tag.tag_name);
        return;
    }

    generate_text_node_if_needed();

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

    generate_text_node_if_needed();

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
    if (!open_elements_.empty()) {
        spdlog::warn("EOF reached with [{}] elements still open", open_elements_.size());
    }
}

void Parser::generate_text_node_if_needed() {
    assert(!open_elements_.empty());
    auto text = std::exchange(current_text_, {}).str();
    bool is_uninteresting = std::all_of(cbegin(text), cend(text), [](char c) { return util::is_whitespace(c); });
    if (is_uninteresting) {
        return;
    }

    open_elements_.top()->children.emplace_back(dom::Text{std::move(text)});
}

} // namespace html
