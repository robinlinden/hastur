// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/tree_constructor.h"

#include "dom2/document_type.h"

#include <spdlog/spdlog.h>

#include <exception>
#include <functional>
#include <string_view>

using namespace std::literals;

namespace html2 {

void TreeConstructor::run(std::string_view input) {
    Tokenizer tokenizer{input, std::bind_front(&TreeConstructor::on_token, this)};
    tokenizer.run();
}

void TreeConstructor::run(std::vector<Token> tokens) {
    auto dummy = Tokenizer(""sv, [](auto &, auto &&) {});
    for (auto &token : tokens) {
        on_token(dummy, std::move(token));
    }
}

void TreeConstructor::on_token(Tokenizer &, Token &&token) {
    spdlog::error("{}: {}", mode_, to_string(token));
    switch (mode_) {
        // https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
        case InsertionMode::Initial: {
            if (auto const *character = std::get_if<CharacterToken>(&token)) {
                switch (character->data) {
                    case '\t':
                    case '\r':
                    case '\f':
                    case '\n':
                    case ' ':
                        return;
                    default:
                        break;
                }
            }

            if (auto const *comment = std::get_if<CommentToken>(&token)) {
                (void)comment;
                std::terminate();
            }

            if (auto const *doctype = std::get_if<DoctypeToken>(&token)) {
                if (doctype->name != "html" || doctype->public_identifier
                        || (doctype->system_identifier && doctype->system_identifier != "about:legacy-compat")) {
                    // Parse error.
                }

                document_->append_child(std::make_shared<dom2::DocumentType>(doctype->name.value_or(""),
                        doctype->public_identifier.value_or(""),
                        doctype->system_identifier.value_or("")));

                // TODO(robinlinden): Quirks checks.
                mode_ = InsertionMode::BeforeHtml;
                return;
            }

            std::terminate();
        }
        // https://html.spec.whatwg.org/multipage/parsing.html#the-before-html-insertion-mode
        case InsertionMode::BeforeHtml: {
            if (std::get_if<DoctypeToken>(&token) != nullptr) {
                // Parse error.
                return;
            }

            if (auto const *comment = std::get_if<CommentToken>(&token)) {
                (void)comment;
                std::terminate();
            }

            if (auto const *character = std::get_if<CharacterToken>(&token)) {
                switch (character->data) {
                    case '\t':
                    case '\r':
                    case '\f':
                    case '\n':
                    case ' ':
                        return;
                    default:
                        break;
                }
            }

            if (auto const *start_tag = std::get_if<StartTagToken>(&token);
                    start_tag && start_tag->tag_name == "html") {
                auto element = document_->append_child(create_element_for_token(token, ns::html, *document_));
                open_elements_.push(element);
                mode_ = InsertionMode::BeforeHead;
                return;
            }

            std::terminate();
        }
        // https://html.spec.whatwg.org/multipage/parsing.html#the-before-head-insertion-mode
        case InsertionMode::BeforeHead: {
            if (auto const *character = std::get_if<CharacterToken>(&token)) {
                switch (character->data) {
                    case '\t':
                    case '\r':
                    case '\f':
                    case '\n':
                    case ' ':
                        return;
                    default:
                        break;
                }
            }

            if (auto const *start_tag = std::get_if<StartTagToken>(&token);
                    start_tag && start_tag->tag_name == "head") {
                insert_html_element(token);
                // TODO(robinlinden): Set the head element pointer to the newly created head element.
                mode_ = InsertionMode::InHead;
                return;
            }

            std::terminate();
        }
        // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead
        case InsertionMode::InHead: {
            if (auto const *character = std::get_if<CharacterToken>(&token)) {
                switch (character->data) {
                    case '\t':
                    case '\r':
                    case '\f':
                    case '\n':
                    case ' ':
                        return;
                    default:
                        break;
                }
            }

            std::terminate();
        }
        default:
            std::terminate();
    }
}

// https://html.spec.whatwg.org/multipage/parsing.html#create-an-element-for-the-token
std::shared_ptr<dom2::Element> TreeConstructor::create_element_for_token(
        Token const &token, std::string_view given_namespace, dom2::Node const &intended_parent) const {
    (void)intended_parent;
    // TODO(robinlinden): Everything.

    // 4. Let local name be the tag name of the token.
    auto local_name = [](Token const &t) {
        if (auto const *start = std::get_if<StartTagToken>(&t)) {
            return start->tag_name;
        } else if (auto const *end = std::get_if<EndTagToken>(&t)) {
            return end->tag_name;
        }
        std::terminate();
    }(token);

    return create_element(*document_, std::move(local_name), given_namespace);
}

// https://dom.spec.whatwg.org/#concept-create-element
std::shared_ptr<dom2::Element> TreeConstructor::create_element([[maybe_unused]] dom2::Document const &document,
        std::string local_name,
        [[maybe_unused]] std::string_view ns,
        [[maybe_unused]] std::optional<std::string_view> prefix,
        [[maybe_unused]] std::optional<std::string_view> is_value,
        [[maybe_unused]] bool synchronous_custom_elements) const {
    // 1. If prefix was not given, let prefix be null.
    // 2. If is was not given, let is be null.
    // 3. Let result be null.
    std::shared_ptr<dom2::Element> result{nullptr};

    // TODO(robinlinden): Everything.
    result = std::make_shared<dom2::Element>(local_name);

    return result;
}

// https://html.spec.whatwg.org/multipage/parsing.html#insert-a-foreign-element
std::shared_ptr<dom2::Element> TreeConstructor::insert_foreign_element(Token const &token, std::string_view ns) {
    // 1. Let the adjusted insertion location be the appropriate place for inserting a node.
    auto &adjusted_insertion_location = appropriate_place_for_inserting_a_node();

    // 2. Let element be the result of creating an element for the token in the
    // given namespace, with the intended parent being the element in which the
    // adjusted insertion location finds itself.
    auto element = create_element_for_token(token, ns, adjusted_insertion_location);

    // 3. If it is possible to insert element at the adjusted insertion location, then:
    if (is_possible_to_insert_element_at(*element, adjusted_insertion_location)) {
        // 1. If the parser was not created as part of the HTML fragment parsing
        // algorithm, then push a new element queue onto element's relevant
        // agent's custom element reactions stack.
        // TODO(robinlinden)

        // 2. Insert element at the adjusted insertion location.
        adjusted_insertion_location.append_child(element);

        // 3. If the parser was not created as part of the HTML fragment parsing
        // algorithm, then pop the element queue from element's relevant agent's
        // custom element reactions stack, and invoke custom element reactions
        // in that queue.
        // TODO(robinlinden)
    }

    // Note: If the adjusted insertion location cannot accept more elements,
    // e.g. because it's a Document that already has an element child, then
    // element is dropped on the floor.

    // 4. Push element onto the stack of open elements so that it is the new
    // current node.
    open_elements_.push(element);

    // 5. Return element.
    return element;
}

// https://html.spec.whatwg.org/multipage/parsing.html#appropriate-place-for-inserting-a-node
dom2::Node &TreeConstructor::appropriate_place_for_inserting_a_node(
        std::optional<std::reference_wrapper<dom2::Node>> override_target) {
    // 1. If there was an override target specified, then let target be the override target.
    // Otherwise, let target be the current node.
    auto &target = override_target ? override_target->get() : current_node();

    // TODO(robinlinden): Almost everything.

    return target;
}

} // namespace html2
