// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSER_H_
#define HTML_PARSER_H_

#include "html/parse_error.h"
#include "html/parser_actions.h"
#include "html/parser_options.h"
#include "html/parser_states.h"
#include "html/token.h"
#include "html/tokenizer.h"

#include "dom/dom.h"

#include <functional>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace html {

class Parser {
public:
    [[nodiscard]] static dom::Document parse_document(
            std::string_view input, ParserOptions const &opts, Callbacks const &cbs) {
        Parser parser{input, opts, cbs};
        return parser.run();
    }

private:
    Parser(std::string_view input, ParserOptions const &opts, Callbacks const &cbs)
        : tokenizer_{input,
                  [this](Tokenizer &tokenizer, Token &&token) { on_token(tokenizer, std::move(token)); },
                  [&cbs](Tokenizer &, ParseError err) {
                      if (!cbs.on_error) {
                          return;
                      }

                      cbs.on_error(err);
                  }},
          scripting_{opts.scripting}, include_comments_{opts.include_comments}, cbs_{cbs} {}

    [[nodiscard]] dom::Document run() {
        tokenizer_.run();
        while (!open_elements_.empty()) {
            actions_.pop_current_node();
        }

        return std::move(doc_);
    }

    void on_token(Tokenizer &, Token &&token) {
        insertion_mode_ = std::visit([&](auto &mode) { return mode.process(actions_, token); }, insertion_mode_)
                                  .value_or(insertion_mode_);
    }

    Tokenizer tokenizer_;
    dom::Document doc_{};
    std::vector<dom::Element *> open_elements_;
    bool scripting_{false};
    bool include_comments_{false};
    Callbacks const &cbs_;
    InsertionMode insertion_mode_;
    Actions actions_{
            doc_,
            tokenizer_,
            scripting_,
            include_comments_ ? CommentMode::Keep : CommentMode::Discard,
            insertion_mode_,
            open_elements_,
            cbs_.on_element_closed,
    };
};

} // namespace html

#endif
