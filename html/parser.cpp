// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/parser.h"

#include "html2/parser_states.h"
#include "html2/token.h"
#include "html2/tokenizer.h"

#include <variant>

namespace html {

void Parser::on_token(html2::Tokenizer &, html2::Token &&token) {
    insertion_mode_ = std::visit([&](auto &mode) { return mode.process(actions_, token); }, insertion_mode_)
                              .value_or(insertion_mode_);
}

} // namespace html
