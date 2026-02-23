// SPDX-FileCopyrightText: 2021-2026 Robin Lindén <dev@robinlinden.eu>
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

#include <algorithm>
#include <array>
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

    [[nodiscard]] static dom::DocumentFragment parse_fragment(
            dom::Element const &context, std::string_view input, ParserOptions const &opts, Callbacks const &cbs) {
        // TODO(robinlinden): Quirks nonsense.
        // TODO(robinlinden): Shadow roots bits.
        Parser parser{input, opts, cbs};

        static constexpr auto kRcdataElements = std::to_array<std::string_view>({"title", "textarea"});
        static constexpr auto kRawtextElements =
                std::to_array<std::string_view>({"style", "xmp", "iframe", "noembed", "noframes"});

        if (std::ranges::contains(kRcdataElements, context.name)) {
            parser.tokenizer_.set_state(State::Rcdata);
        } else if (std::ranges::contains(kRawtextElements, context.name)) {
            parser.tokenizer_.set_state(State::Rawtext);
        } else if (context.name == "script") {
            parser.tokenizer_.set_state(State::ScriptData);
        } else if (context.name == "noscript") {
            if (opts.scripting) {
                parser.tokenizer_.set_state(State::Rawtext);
            }
        } else if (context.name == "plaintext") {
            parser.tokenizer_.set_state(State::Plaintext);
        }

        auto &html = parser.doc_.html();
        html.name = "html";
        parser.open_elements_.push_back(&html);

        // TODO(robinlinden): Template stuff.

        parser.actions_.set_fragment_parsing_context(context.name);
        parser.insertion_mode_ = appropriate_insertion_mode(parser.actions_);

        // TODO(robinlinden): Form whatever.

        auto res = parser.run();

        return dom::DocumentFragment{
                .children = std::move(res.html().children),
        };
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
