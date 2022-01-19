// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML2_TREE_BUILDER_H_
#define HTML2_TREE_BUILDER_H_

#include "dom2/document.h"
#include "dom2/element.h"
#include "dom2/node.h"
#include "html2/tokenizer.h"

#include <functional>
#include <memory>
#include <optional>
#include <stack>
#include <string_view>

namespace html2 {
namespace ns {
static constexpr auto html = std::string_view{"http://www.w3.org/1999/xhtml"};
} // namespace ns

enum class InsertionMode {
    Initial,
    BeforeHtml,
    BeforeHead,
    InHead,
    InHeadNoscript,
    AfterHead,
    InBody,
    Text,
    InTable,
    InTableText,
    InCaption,
    InColumnGroup,
    InTableBody,
    InRow,
    InCell,
    InSelect,
    InSelectInTable,
    InTemplate,
    AfterBody,
    InFrameset,
    AfterFrameset,
    AfterAfterBody,
    AfterAfterFrameset,
};

class TreeConstructor {
public:
    void run(std::string_view input);

private:
    InsertionMode mode_{InsertionMode::Initial};
    std::unique_ptr<dom2::Document> document_{std::make_unique<dom2::Document>()};
    std::stack<std::shared_ptr<dom2::Node>> open_elements_{};

    void on_token(Tokenizer &, Token &&);

    std::shared_ptr<dom2::Element> create_element_for_token(
            Token const &, std::string_view given_namespace, dom2::Node const &intended_parent) const;

    std::shared_ptr<dom2::Element> create_element(dom2::Document const &,
            std::string local_name,
            std::string_view ns,
            std::optional<std::string_view> prefix = std::nullopt,
            std::optional<std::string_view> is = std::nullopt,
            bool synchronous_custom_elements = false) const;

    std::shared_ptr<dom2::Element> insert_foreign_element(Token const &, std::string_view ns);
    std::shared_ptr<dom2::Element> insert_html_element(Token const &token) {
        return insert_foreign_element(token, ns::html);
    }

    dom2::Node &appropriate_place_for_inserting_a_node(
            std::optional<std::reference_wrapper<dom2::Node>> override_target = std::nullopt);

    dom2::Node &current_node() {
        // It's UB to call back() on an empty std::deque.
        if (open_elements_.empty()) {
            std::terminate();
        }

        return *open_elements_.top();
    }

    bool is_possible_to_insert_element_at(dom2::Element const &, [[maybe_unused]] dom2::Node const &at) const {
        // TODO(robinlinden):
        // If the adjusted insertion location cannot accept more elements, e.g.
        // because it's a Document that already has an element child.
        return true;
    }
};

} // namespace html2

#endif
