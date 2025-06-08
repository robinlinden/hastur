// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/tokenizer.h"

#include "html2/character_reference.h"
#include "html2/parse_error.h"
#include "html2/token.h"

#include "unicode/util.h"
#include "util/string.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

namespace html2 {
namespace {

constexpr bool is_c0_control(int code_point) {
    return code_point >= 0x00 && code_point <= 0x1F;
}

constexpr bool is_control(int code_point) {
    return is_c0_control(code_point) || (code_point >= 0x7F && code_point <= 0x9F);
}

constexpr bool is_ascii_whitespace(int code_point) {
    switch (code_point) {
        case 0x09:
        case 0x0A:
        case 0x0C:
        case 0x0D:
        case 0x20:
            return true;
        default:
            return false;
    }
}

constexpr auto kReplacementCharacter = "\xEF\xBF\xBD"sv;

} // namespace

void Tokenizer::set_state(State state) {
    state_ = state;
}

// While long, this function only contains trivial and short cases for each of
// the parser states.
//
// Splitting it would (very slightly) complicate stopping parsing as instead of
// just returning when we're done, we'd have to keep a member keeping track of
// if we're done and check that after every state, or return an enum value
// telling us if we should continue or return.
// NOLINTNEXTLINE(google-readability-function-size)
void Tokenizer::run() {
    while (true) {
        switch (state_) {
            // https://html.spec.whatwg.org/multipage/parsing.html#data-state
            case State::Data: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '&':
                        return_state_ = State::Data;
                        state_ = State::CharacterReference;
                        continue;
                    case '<':
                        state_ = State::TagOpen;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        emit(CharacterToken{*c});
                        continue;
                    default:
                        emit(CharacterToken{*c});
                        continue;
                }
                break;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#rcdata-state
            case State::Rcdata: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '&':
                        return_state_ = State::Rcdata;
                        state_ = State::CharacterReference;
                        continue;
                    case '<':
                        state_ = State::RcdataLessThanSign;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        emit_replacement_character();
                        continue;
                    default:
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#rawtext-state
            case State::Rawtext: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '<':
                        state_ = State::RawtextLessThanSign;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        emit_replacement_character();
                        continue;
                    default:
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            case State::ScriptData: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '<':
                        state_ = State::ScriptDataLessThanSign;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        emit_replacement_character();
                        continue;
                    default:
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#plaintext-state
            case State::Plaintext: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(EndOfFileToken{});
                    return;
                }

                if (c == '\0') {
                    emit(ParseError::UnexpectedNullCharacter);
                    emit_replacement_character();
                    continue;
                }

                emit(CharacterToken{*c});
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#tag-open-state
            case State::TagOpen: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofBeforeTagName);
                    emit(CharacterToken{'<'});
                    emit(EndOfFileToken{});
                    return;
                }

                if (util::is_alpha(*c)) {
                    current_token_ = StartTagToken{};
                    reconsume_in(State::TagName);
                    continue;
                }

                switch (*c) {
                    case '!':
                        state_ = State::MarkupDeclarationOpen;
                        continue;
                    case '/':
                        state_ = State::EndTagOpen;
                        continue;
                    case '?':
                        emit(ParseError::UnexpectedQuestionMarkInsteadOfTagName);
                        current_token_ = CommentToken{""};
                        reconsume_in(State::BogusComment);
                        continue;
                    default:
                        emit(ParseError::InvalidFirstCharacterOfTagName);
                        emit(CharacterToken{'<'});
                        reconsume_in(State::Data);
                        continue;
                }
                break;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#end-tag-open-state
            case State::EndTagOpen: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofBeforeTagName);
                    emit(CharacterToken{'<'});
                    emit(CharacterToken{'/'});
                    emit(EndOfFileToken{});
                    return;
                }

                if (util::is_alpha(*c)) {
                    current_token_ = EndTagToken{};
                    reconsume_in(State::TagName);
                    continue;
                }

                if (c == '>') {
                    emit(ParseError::MissingEndTagName);
                    state_ = State::Data;
                    continue;
                }

                emit(ParseError::InvalidFirstCharacterOfTagName);
                current_token_ = CommentToken{};
                reconsume_in(State::BogusComment);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#tag-name-state
            case State::TagName: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInTag);
                    emit(EndOfFileToken{});
                    return;
                }

                auto append_to_tag_name = [&](auto text) {
                    if (auto *start_tag = std::get_if<StartTagToken>(&current_token_)) {
                        start_tag->tag_name += text;
                    } else {
                        std::get<EndTagToken>(current_token_).tag_name += text;
                    }
                };

                if (util::is_upper_alpha(*c)) {
                    append_to_tag_name(util::lowercased(*c));
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        state_ = State::BeforeAttributeName;
                        continue;
                    case '/':
                        state_ = State::SelfClosingStartTag;
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        append_to_tag_name(kReplacementCharacter);
                        continue;
                    default:
                        append_to_tag_name(*c);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#rcdata-less-than-sign-state
            case State::RcdataLessThanSign: {
                if (auto c = consume_next_input_character(); c == '/') {
                    temporary_buffer_.clear();
                    state_ = State::RcdataEndTagOpen;
                    continue;
                }

                emit(CharacterToken{'<'});
                reconsume_in(State::Rcdata);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#rcdata-end-tag-open-state
            case State::RcdataEndTagOpen: {
                if (auto c = consume_next_input_character(); c && util::is_alpha(*c)) {
                    current_token_ = EndTagToken{};
                    reconsume_in(State::RcdataEndTagName);
                    continue;
                }

                emit(CharacterToken{'<'});
                emit(CharacterToken{'/'});
                reconsume_in(State::Rcdata);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#rcdata-end-tag-name-state
            case State::RcdataEndTagName: {
                auto anything_else = [&] {
                    emit(CharacterToken{'<'});
                    emit(CharacterToken{'/'});
                    emit_temporary_buffer_as_character_tokens();
                    reconsume_in(State::Rcdata);
                };

                auto c = consume_next_input_character();
                if (!c) {
                    anything_else();
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::BeforeAttributeName;
                            continue;
                        }
                        anything_else();
                        continue;
                    case '/':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::SelfClosingStartTag;
                            continue;
                        }
                        anything_else();
                        continue;
                    case '>':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::Data;
                            emit(std::move(current_token_));
                            continue;
                        }
                        anything_else();
                        continue;
                    default:
                        break;
                }

                if (util::is_alpha(*c)) {
                    std::get<EndTagToken>(current_token_).tag_name += util::lowercased(*c);
                    temporary_buffer_ += *c;
                    continue;
                }

                anything_else();
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#rawtext-less-than-sign-state
            case State::RawtextLessThanSign: {
                if (auto c = consume_next_input_character(); c == '/') {
                    temporary_buffer_.clear();
                    state_ = State::RawtextEndTagOpen;
                    continue;
                }

                emit(CharacterToken{'<'});
                reconsume_in(State::Rawtext);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#rawtext-end-tag-open-state
            case State::RawtextEndTagOpen: {
                if (auto c = consume_next_input_character(); c && util::is_alpha(*c)) {
                    current_token_ = EndTagToken{};
                    reconsume_in(State::RawtextEndTagName);
                    continue;
                }

                emit(CharacterToken{'<'});
                emit(CharacterToken{'/'});
                reconsume_in(State::Rawtext);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#rawtext-end-tag-name-state
            case State::RawtextEndTagName: {
                auto anything_else = [this] {
                    emit(CharacterToken{'<'});
                    emit(CharacterToken{'/'});
                    for (char ch : temporary_buffer_) {
                        emit(CharacterToken{ch});
                    }
                    reconsume_in(State::Rawtext);
                };

                auto c = consume_next_input_character();
                if (!c) {
                    anything_else();
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::BeforeAttributeName;
                            continue;
                        }
                        anything_else();
                        continue;
                    case '/':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::SelfClosingStartTag;
                            continue;
                        }
                        anything_else();
                        continue;
                    case '>':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::Data;
                            emit(std::move(current_token_));
                            continue;
                        }
                        anything_else();
                        continue;
                    default:
                        break;
                }

                if (util::is_alpha(*c)) {
                    std::get<EndTagToken>(current_token_).tag_name += util::lowercased(*c);
                    temporary_buffer_ += *c;
                    continue;
                }

                anything_else();
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-less-than-sign-state
            case State::ScriptDataLessThanSign: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(CharacterToken{'<'});
                    reconsume_in(State::ScriptData);
                    continue;
                }

                switch (*c) {
                    case '/':
                        temporary_buffer_ = "";
                        state_ = State::ScriptDataEndTagOpen;
                        continue;
                    case '!':
                        state_ = State::ScriptDataEscapeStart;
                        emit(CharacterToken{'<'});
                        emit(CharacterToken{'!'});
                        continue;
                    default:
                        emit(CharacterToken{'<'});
                        reconsume_in(State::ScriptData);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-end-tag-open-state
            case State::ScriptDataEndTagOpen: {
                auto c = consume_next_input_character();
                if (c && util::is_alpha(*c)) {
                    current_token_ = EndTagToken{};
                    reconsume_in(State::ScriptDataEndTagName);
                    continue;
                }

                emit(CharacterToken{'<'});
                emit(CharacterToken{'/'});
                reconsume_in(State::ScriptData);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-end-tag-name-state
            case State::ScriptDataEndTagName: {
                auto c = consume_next_input_character();

                auto anything_else = [this] {
                    emit(CharacterToken{'<'});
                    emit(CharacterToken{'/'});
                    emit_temporary_buffer_as_character_tokens();
                    reconsume_in(State::ScriptData);
                };

                if (!c) {
                    anything_else();
                    continue;
                }

                if (util::is_upper_alpha(*c)) {
                    std::get<EndTagToken>(current_token_).tag_name.append(1, util::lowercased(*c));
                    temporary_buffer_.append(1, *c);
                    continue;
                }

                if (util::is_lower_alpha(*c)) {
                    std::get<EndTagToken>(current_token_).tag_name.append(1, *c);
                    temporary_buffer_.append(1, *c);
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::BeforeAttributeName;
                        } else {
                            anything_else();
                        }
                        continue;
                    case '/':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::SelfClosingStartTag;
                        } else {
                            anything_else();
                        }
                        continue;
                    case '>':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::Data;
                            emit(std::move(current_token_));
                        } else {
                            anything_else();
                        }
                        continue;
                    default:
                        anything_else();
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-escape-start-state
            case State::ScriptDataEscapeStart: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::ScriptData);
                    continue;
                }

                switch (*c) {
                    case '-':
                        state_ = State::ScriptDataEscapeStartDash;
                        emit(CharacterToken{*c});
                        continue;
                    default:
                        reconsume_in(State::ScriptData);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-escape-start-dash-state
            case State::ScriptDataEscapeStartDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::ScriptData);
                    continue;
                }

                switch (*c) {
                    case '-':
                        state_ = State::ScriptDataEscapedDashDash;
                        emit(CharacterToken{*c});
                        continue;
                    default:
                        reconsume_in(State::ScriptData);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-escaped-state
            case State::ScriptDataEscaped: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInScriptHtmlCommentLikeText);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        state_ = State::ScriptDataEscapedDash;
                        emit(CharacterToken{*c});
                        continue;
                    case '<':
                        state_ = State::ScriptDataEscapedLessThanSign;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        emit_replacement_character();
                        continue;
                    default:
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-escaped-dash-state
            case State::ScriptDataEscapedDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInScriptHtmlCommentLikeText);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        state_ = State::ScriptDataEscapedDashDash;
                        emit(CharacterToken{*c});
                        continue;
                    case '<':
                        state_ = State::ScriptDataEscapedLessThanSign;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        state_ = State::ScriptDataEscaped;
                        emit_replacement_character();
                        continue;
                    default:
                        state_ = State::ScriptDataEscaped;
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-escaped-dash-dash-state
            case State::ScriptDataEscapedDashDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInScriptHtmlCommentLikeText);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        emit(CharacterToken{*c});
                        continue;
                    case '<':
                        state_ = State::ScriptDataEscapedLessThanSign;
                        continue;
                    case '>':
                        state_ = State::ScriptData;
                        emit(CharacterToken{*c});
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        state_ = State::ScriptDataEscaped;
                        emit_replacement_character();
                        continue;
                    default:
                        state_ = State::ScriptDataEscaped;
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-escaped-less-than-sign-state
            case State::ScriptDataEscapedLessThanSign: {
                auto c = consume_next_input_character();
                if (c == '/') {
                    temporary_buffer_ = "";
                    state_ = State::ScriptDataEscapedEndTagOpen;
                    continue;
                }

                if (c && util::is_alpha(*c)) {
                    temporary_buffer_ = "";
                    emit(CharacterToken{'<'});
                    reconsume_in(State::ScriptDataDoubleEscapeStart);
                    continue;
                }

                emit(CharacterToken{'<'});
                reconsume_in(State::ScriptDataEscaped);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-escaped-end-tag-open-state
            case State::ScriptDataEscapedEndTagOpen: {
                auto c = consume_next_input_character();
                if (c && util::is_alpha(*c)) {
                    current_token_ = EndTagToken{};
                    reconsume_in(State::ScriptDataEscapedEndTagName);
                    continue;
                }

                emit(CharacterToken{'<'});
                emit(CharacterToken{'/'});
                reconsume_in(State::ScriptDataEscaped);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-escaped-end-tag-name-state
            case State::ScriptDataEscapedEndTagName: {
                auto c = consume_next_input_character();

                auto anything_else = [this] {
                    emit(CharacterToken{'<'});
                    emit(CharacterToken{'/'});
                    emit_temporary_buffer_as_character_tokens();
                    reconsume_in(State::ScriptDataEscaped);
                };

                if (!c) {
                    anything_else();
                    continue;
                }

                if (util::is_upper_alpha(*c)) {
                    std::get<EndTagToken>(current_token_).tag_name.append(1, util::lowercased(*c));
                    temporary_buffer_.append(1, *c);
                    continue;
                }

                if (util::is_lower_alpha(*c)) {
                    std::get<EndTagToken>(current_token_).tag_name.append(1, *c);
                    temporary_buffer_.append(1, *c);
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::BeforeAttributeName;
                        } else {
                            anything_else();
                        }
                        continue;
                    case '/':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::SelfClosingStartTag;
                        } else {
                            anything_else();
                        }
                        continue;
                    case '>':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::Data;
                            emit(std::move(current_token_));
                        } else {
                            anything_else();
                        }
                        continue;
                    default:
                        anything_else();
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-double-escape-start-state
            case State::ScriptDataDoubleEscapeStart: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::ScriptDataEscaped);
                    continue;
                }

                if (util::is_upper_alpha(*c)) {
                    temporary_buffer_.append(1, util::lowercased(*c));
                    emit(CharacterToken{*c});
                    continue;
                }

                if (util::is_lower_alpha(*c)) {
                    temporary_buffer_.append(1, *c);
                    emit(CharacterToken{*c});
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                    case '/':
                    case '>':
                        if (temporary_buffer_ == "script"sv) {
                            state_ = State::ScriptDataDoubleEscaped;
                        } else {
                            state_ = State::ScriptDataEscaped;
                        }
                        emit(CharacterToken{*c});
                        continue;
                    default:
                        reconsume_in(State::ScriptDataEscaped);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-double-escaped-state
            case State::ScriptDataDoubleEscaped: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInScriptHtmlCommentLikeText);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        state_ = State::ScriptDataDoubleEscapedDash;
                        emit(CharacterToken{*c});
                        continue;
                    case '<':
                        state_ = State::ScriptDataDoubleEscapedLessThanSign;
                        emit(CharacterToken{*c});
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        emit_replacement_character();
                        continue;
                    default:
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-double-escaped-dash-state
            case State::ScriptDataDoubleEscapedDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInScriptHtmlCommentLikeText);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        state_ = State::ScriptDataDoubleEscapedDashDash;
                        emit(CharacterToken{*c});
                        continue;
                    case '<':
                        state_ = State::ScriptDataDoubleEscapedLessThanSign;
                        emit(CharacterToken{*c});
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        state_ = State::ScriptDataDoubleEscaped;
                        emit_replacement_character();
                        continue;
                    default:
                        state_ = State::ScriptDataDoubleEscaped;
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-double-escaped-dash-dash-state
            case State::ScriptDataDoubleEscapedDashDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInScriptHtmlCommentLikeText);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        emit(CharacterToken{*c});
                        continue;
                    case '<':
                        state_ = State::ScriptDataDoubleEscapedLessThanSign;
                        emit(CharacterToken{*c});
                        continue;
                    case '>':
                        state_ = State::ScriptData;
                        emit(CharacterToken{*c});
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        state_ = State::ScriptDataDoubleEscaped;
                        emit_replacement_character();
                        continue;
                    default:
                        state_ = State::ScriptDataDoubleEscaped;
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-double-escaped-less-than-sign-state
            case State::ScriptDataDoubleEscapedLessThanSign: {
                auto c = consume_next_input_character();
                if (c == '/') {
                    temporary_buffer_ = "";
                    state_ = State::ScriptDataDoubleEscapeEnd;
                    emit(CharacterToken{*c});
                    continue;
                }

                reconsume_in(State::ScriptDataDoubleEscaped);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#script-data-double-escape-end-state
            case State::ScriptDataDoubleEscapeEnd: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::ScriptDataDoubleEscaped);
                    continue;
                }

                if (util::is_upper_alpha(*c)) {
                    temporary_buffer_.append(1, util::lowercased(*c));
                    emit(CharacterToken{*c});
                    continue;
                }

                if (util::is_lower_alpha(*c)) {
                    temporary_buffer_.append(1, *c);
                    emit(CharacterToken{*c});
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                    case '/':
                    case '>':
                        if (temporary_buffer_ == "script"sv) {
                            state_ = State::ScriptDataEscaped;
                        } else {
                            state_ = State::ScriptDataDoubleEscaped;
                        }
                        emit(CharacterToken{*c});
                        continue;
                    default:
                        reconsume_in(State::ScriptDataDoubleEscaped);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#before-attribute-name-state
            case State::BeforeAttributeName: {
                auto c = consume_next_input_character();
                if (!c || *c == '/' || *c == '>') {
                    reconsume_in(State::AfterAttributeName);
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        continue;
                    case '=':
                        emit(ParseError::UnexpectedEqualsSignBeforeAttributeName);
                        start_attribute_in_current_tag_token({.name = "="});
                        state_ = State::AttributeName;
                        continue;
                    default:
                        start_attribute_in_current_tag_token({});
                        reconsume_in(State::AttributeName);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#attribute-name-state
            case State::AttributeName: {
                auto warn_on_duplicate_attributes = [&] {
                    auto const &all_attrs = attributes_for_current_element();
                    auto const &current_attr = all_attrs.back();
                    for (std::size_t i = 0; i < all_attrs.size() - 1; ++i) {
                        if (all_attrs[i].name == current_attr.name) {
                            emit(ParseError::DuplicateAttribute);
                            break;
                        }
                    }
                };

                auto c = consume_next_input_character();
                if (!c || *c == '\t' || *c == '\n' || *c == '\f' || *c == ' ' || *c == '/' || *c == '>') {
                    warn_on_duplicate_attributes();
                    reconsume_in(State::AfterAttributeName);
                    continue;
                }

                auto append_to_current_attribute_name = [&](auto text) {
                    attributes_for_current_element().back().name += text;
                };

                if (util::is_upper_alpha(*c)) {
                    append_to_current_attribute_name(util::lowercased(*c));
                    continue;
                }

                switch (*c) {
                    case '=':
                        warn_on_duplicate_attributes();
                        state_ = State::BeforeAttributeValue;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        append_to_current_attribute_name(kReplacementCharacter);
                        continue;
                    case '"':
                    case '\'':
                    case '<':
                        emit(ParseError::UnexpectedCharacterInAttributeName);
                        [[fallthrough]];
                    default:
                        append_to_current_attribute_name(*c);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#after-attribute-name-state
            case State::AfterAttributeName: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInTag);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        continue;
                    case '/':
                        state_ = State::SelfClosingStartTag;
                        continue;
                    case '=':
                        state_ = State::BeforeAttributeValue;
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        start_attribute_in_current_tag_token({});
                        reconsume_in(State::AttributeName);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#before-attribute-value-state
            case State::BeforeAttributeValue: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::AttributeValueUnquoted);
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        continue;
                    case '"':
                        state_ = State::AttributeValueDoubleQuoted;
                        continue;
                    case '\'':
                        state_ = State::AttributeValueSingleQuoted;
                        continue;
                    case '>':
                        emit(ParseError::MissingAttributeValue);
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        reconsume_in(State::AttributeValueUnquoted);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#attribute-value-(double-quoted)-state
            case State::AttributeValueDoubleQuoted: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInTag);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '"':
                        state_ = State::AfterAttributeValueQuoted;
                        continue;
                    case '&':
                        return_state_ = State::AttributeValueDoubleQuoted;
                        state_ = State::CharacterReference;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        current_attribute().value += kReplacementCharacter;
                        continue;
                    default:
                        current_attribute().value += *c;
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#attribute-value-(single-quoted)-state
            case State::AttributeValueSingleQuoted: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInTag);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\'':
                        state_ = State::AfterAttributeValueQuoted;
                        continue;
                    case '&':
                        return_state_ = State::AttributeValueSingleQuoted;
                        state_ = State::CharacterReference;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        current_attribute().value += kReplacementCharacter;
                        continue;
                    default:
                        current_attribute().value += *c;
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#attribute-value-(unquoted)-state
            case State::AttributeValueUnquoted: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInTag);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        state_ = State::BeforeAttributeName;
                        continue;
                    case '&':
                        return_state_ = State::AttributeValueUnquoted;
                        state_ = State::CharacterReference;
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        current_attribute().value += kReplacementCharacter;
                        continue;
                    case '"':
                    case '\'':
                    case '<':
                    case '=':
                    case '`':
                        emit(ParseError::UnexpectedCharacterInUnquotedAttributeValue);
                        [[fallthrough]];
                    default:
                        current_attribute().value += *c;
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#after-attribute-value-(quoted)-state
            case State::AfterAttributeValueQuoted: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInTag);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        state_ = State::BeforeAttributeName;
                        continue;
                    case '/':
                        state_ = State::SelfClosingStartTag;
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        emit(ParseError::MissingWhitespaceBetweenAttributes);
                        reconsume_in(State::BeforeAttributeName);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#self-closing-start-tag-state
            case State::SelfClosingStartTag: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInTag);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '>':
                        if (auto *start_tag = std::get_if<StartTagToken>(&current_token_)) {
                            start_tag->self_closing = true;
                        } else {
                            self_closing_end_tag_detected_ = true;
                        }
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        emit(ParseError::UnexpectedSolidusInTag);
                        reconsume_in(State::BeforeAttributeName);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#bogus-comment-state
            case State::BogusComment: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        std::get<CommentToken>(current_token_).data += kReplacementCharacter;
                        continue;
                    default:
                        std::get<CommentToken>(current_token_).data += *c;
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#markup-declaration-open-state
            case State::MarkupDeclarationOpen:
                if (input_.substr(pos_, 2) == "--") {
                    pos_ += 2;
                    current_token_ = CommentToken{.data = std::string{}};
                    state_ = State::CommentStart;
                    continue;
                }

                if (util::no_case_compare(input_.substr(pos_, std::strlen("DOCTYPE")), "doctype"sv)) {
                    pos_ += std::strlen("DOCTYPE");
                    state_ = State::Doctype;
                    continue;
                }

                if (input_.substr(pos_, std::strlen("[CDATA[")) == "[CDATA["sv) {
                    pos_ += std::strlen("[CDATA[");
                    if (!adjusted_current_node_in_html_namespace_) {
                        state_ = State::CdataSection;
                        continue;
                    }

                    emit(ParseError::CdataInHtmlContent);
                    current_token_ = CommentToken{.data = "[CDATA["};
                    state_ = State::BogusComment;
                    continue;
                }

                emit(ParseError::IncorrectlyOpenedComment);
                current_token_ = CommentToken{};
                state_ = State::BogusComment;
                continue;

            // https://html.spec.whatwg.org/#comment-start-state
            case State::CommentStart: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::Comment);
                    continue;
                }

                switch (*c) {
                    case '-':
                        state_ = State::CommentStartDash;
                        continue;
                    case '>':
                        emit(ParseError::AbruptClosingOfEmptyComment);
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#comment-start-dash-state
            case State::CommentStartDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInComment);
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        state_ = State::CommentEnd;
                        continue;
                    case '>':
                        emit(ParseError::AbruptClosingOfEmptyComment);
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        std::get<CommentToken>(current_token_).data.append("-");
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#comment-state
            case State::Comment: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInComment);
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '<':
                        std::get<CommentToken>(current_token_).data.append(1, *c);
                        state_ = State::CommentLessThanSign;
                        continue;
                    case '-':
                        state_ = State::CommentEndDash;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        std::get<CommentToken>(current_token_).data += kReplacementCharacter;
                        continue;
                    default:
                        std::get<CommentToken>(current_token_).data.append(1, *c);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#comment-less-than-sign-state
            case State::CommentLessThanSign: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::Comment);
                    continue;
                }

                switch (*c) {
                    case '!':
                        std::get<CommentToken>(current_token_).data.append(1, *c);
                        state_ = State::CommentLessThanSignBang;
                        continue;
                    case '<':
                        std::get<CommentToken>(current_token_).data.append(1, *c);
                        continue;
                    default:
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#comment-less-than-sign-bang-state
            case State::CommentLessThanSignBang: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::Comment);
                    continue;
                }

                switch (*c) {
                    case '-':
                        state_ = State::CommentLessThanSignBangDash;
                        continue;
                    default:
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#comment-less-than-sign-bang-dash-state
            case State::CommentLessThanSignBangDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::CommentEndDash);
                    continue;
                }

                switch (*c) {
                    case '-':
                        state_ = State::CommentLessThanSignBangDashDash;
                        continue;
                    default:
                        reconsume_in(State::CommentEndDash);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#comment-less-than-sign-bang-dash-dash-state
            case State::CommentLessThanSignBangDashDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::CommentEnd);
                    continue;
                }

                switch (*c) {
                    case '>':
                        reconsume_in(State::CommentEnd);
                        continue;
                    default:
                        emit(ParseError::NestedComment);
                        reconsume_in(State::CommentEnd);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#comment-end-dash-state
            case State::CommentEndDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInComment);
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        state_ = State::CommentEnd;
                        continue;
                    default:
                        std::get<CommentToken>(current_token_).data.append("-");
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#comment-end-state
            case State::CommentEnd: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInComment);
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    case '!':
                        state_ = State::CommentEndBang;
                        continue;
                    case '-':
                        std::get<CommentToken>(current_token_).data.append("-");
                        continue;
                    default:
                        std::get<CommentToken>(current_token_).data.append("--");
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#comment-end-bang-state
            case State::CommentEndBang: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInComment);
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        std::get<CommentToken>(current_token_).data.append("--!");
                        state_ = State::CommentEndDash;
                        continue;
                    case '>':
                        emit(ParseError::IncorrectlyClosedComment);
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        std::get<CommentToken>(current_token_).data.append("--!");
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#doctype-state
            case State::Doctype: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    emit(DoctypeToken{.force_quirks = true});
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        state_ = State::BeforeDoctypeName;
                        continue;
                    case '>':
                        reconsume_in(State::BeforeDoctypeName);
                        continue;
                    default:
                        emit(ParseError::MissingWhitespaceBeforeDoctypeName);
                        reconsume_in(State::BeforeDoctypeName);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#before-doctype-name-state
            case State::BeforeDoctypeName: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    emit(DoctypeToken{.force_quirks = true});
                    emit(EndOfFileToken{});
                    return;
                }

                if (util::is_upper_alpha(*c)) {
                    current_token_ = DoctypeToken{.name = std::string{util::lowercased(*c)}};
                    state_ = State::DoctypeName;
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        current_token_ = DoctypeToken{.name = std::string{kReplacementCharacter}};
                        state_ = State::DoctypeName;
                        continue;
                    case '>':
                        emit(ParseError::MissingDoctypeName);
                        current_token_ = DoctypeToken{.force_quirks = true};
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        current_token_ = DoctypeToken{.name = std::string{*c}};
                        state_ = State::DoctypeName;
                        continue;
                }
            }

            // Only reachable via State::BeforeDoctypeName, and every branch
            // there sets current_token_ to DoctypeToken and initalizes
            // DoctypeToken::name to something not std::nullopt.
            // https://html.spec.whatwg.org/#doctype-name-state
            case State::DoctypeName: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                // Help bugprone-unchecked-optional-access a bit.
                auto &doctype_name = std::get<DoctypeToken>(current_token_).name;
                assert(doctype_name.has_value());

                if (util::is_upper_alpha(*c)) {
                    doctype_name->append(1, util::lowercased(*c));
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        state_ = State::AfterDoctypeName;
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        *doctype_name += kReplacementCharacter;
                        continue;
                    default:
                        doctype_name->append(1, *c);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#after-doctype-name-state
            case State::AfterDoctypeName: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        if (util::no_case_compare(input_.substr(pos_ - 1, std::strlen("PUBLIC")), "public"sv)) {
                            pos_ += std::strlen("PUBLIC") - 1;
                            state_ = State::AfterDoctypePublicKeyword;
                            continue;
                        }

                        if (util::no_case_compare(input_.substr(pos_ - 1, std::strlen("SYSTEM")), "system"sv)) {
                            pos_ += std::strlen("SYSTEM") - 1;
                            state_ = State::AfterDoctypeSystemKeyword;
                            continue;
                        }

                        emit(ParseError::InvalidCharacterSequenceAfterDoctypeName);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        reconsume_in(State::BogusDoctype);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#after-doctype-public-keyword-state
            case State::AfterDoctypePublicKeyword: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        state_ = State::BeforeDoctypePublicIdentifier;
                        continue;
                    case '"':
                        emit(ParseError::MissingWhitespaceAfterDoctypePublicKeyword);
                        std::get<DoctypeToken>(current_token_).public_identifier = ""s;
                        state_ = State::DoctypePublicIdentifierDoubleQuoted;
                        continue;
                    case '\'':
                        emit(ParseError::MissingWhitespaceAfterDoctypePublicKeyword);
                        std::get<DoctypeToken>(current_token_).public_identifier = ""s;
                        state_ = State::DoctypePublicIdentifierSingleQuoted;
                        continue;
                    case '>':
                        emit(ParseError::MissingDoctypePublicIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        emit(ParseError::MissingQuoteBeforeDoctypePublicIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        reconsume_in(State::BogusDoctype);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#before-doctype-public-identifier-state
            case State::BeforeDoctypePublicIdentifier: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        continue;
                    case '"':
                        std::get<DoctypeToken>(current_token_).public_identifier = ""s;
                        state_ = State::DoctypePublicIdentifierDoubleQuoted;
                        continue;
                    case '\'':
                        std::get<DoctypeToken>(current_token_).public_identifier = ""s;
                        state_ = State::DoctypePublicIdentifierSingleQuoted;
                        continue;
                    case '>':
                        emit(ParseError::MissingDoctypePublicIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        emit(ParseError::MissingQuoteBeforeDoctypePublicIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        reconsume_in(State::BogusDoctype);
                        continue;
                }
            }

            // Reachable via State::AfterDoctypePublicKeyword and
            // State::BeforeDoctypePublicIdentifier, both of which set
            // DoctypeToken::public_identifier to an empty string.
            // https://html.spec.whatwg.org/#doctype-public-identifier-(double-quoted)-state
            case State::DoctypePublicIdentifierDoubleQuoted: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                // Help bugprone-unchecked-optional-access a bit.
                auto &doctype_public_identifier = std::get<DoctypeToken>(current_token_).public_identifier;
                assert(doctype_public_identifier.has_value());

                switch (*c) {
                    case '"':
                        state_ = State::AfterDoctypePublicIdentifier;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        *doctype_public_identifier += kReplacementCharacter;
                        continue;
                    case '>':
                        emit(ParseError::AbruptDoctypePublicIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        *doctype_public_identifier += *c;
                        continue;
                }
            }

            // Reachable via State::AfterDoctypePublicKeyword and
            // State::BeforeDoctypePublicIdentifier, both of which set
            // DoctypeToken::public_identifier to an empty string.
            // https://html.spec.whatwg.org/#doctype-public-identifier-(single-quoted)-state
            case State::DoctypePublicIdentifierSingleQuoted: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                // Help bugprone-unchecked-optional-access a bit.
                auto &doctype_public_identifier = std::get<DoctypeToken>(current_token_).public_identifier;
                assert(doctype_public_identifier);

                switch (*c) {
                    case '\'':
                        state_ = State::AfterDoctypePublicIdentifier;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        *doctype_public_identifier += kReplacementCharacter;
                        continue;
                    case '>':
                        emit(ParseError::AbruptDoctypePublicIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        *doctype_public_identifier += *c;
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#after-doctype-public-identifier-state
            case State::AfterDoctypePublicIdentifier: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        state_ = State::BetweenDoctypePublicAndSystemIdentifiers;
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    case '"':
                        emit(ParseError::MissingWhitespaceBetweenDoctypePublicAndSystemIdentifiers);
                        std::get<DoctypeToken>(current_token_).system_identifier = "";
                        state_ = State::DoctypeSystemIdentifierDoubleQuoted;
                        continue;
                    case '\'':
                        emit(ParseError::MissingWhitespaceBetweenDoctypePublicAndSystemIdentifiers);
                        std::get<DoctypeToken>(current_token_).system_identifier = "";
                        state_ = State::DoctypeSystemIdentifierSingleQuoted;
                        continue;
                    default:
                        emit(ParseError::MissingQuoteBeforeDoctypeSystemIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        reconsume_in(State::BogusDoctype);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#between-doctype-public-and-system-identifiers-state
            case State::BetweenDoctypePublicAndSystemIdentifiers: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    case '"':
                        std::get<DoctypeToken>(current_token_).system_identifier = "";
                        state_ = State::DoctypeSystemIdentifierDoubleQuoted;
                        continue;
                    case '\'':
                        std::get<DoctypeToken>(current_token_).system_identifier = "";
                        state_ = State::DoctypeSystemIdentifierSingleQuoted;
                        continue;
                    default:
                        emit(ParseError::MissingQuoteBeforeDoctypeSystemIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        reconsume_in(State::BogusDoctype);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#after-doctype-system-keyword-state
            case State::AfterDoctypeSystemKeyword: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        state_ = State::BeforeDoctypeSystemIdentifier;
                        continue;
                    case '"':
                        emit(ParseError::MissingWhitespaceAfterDoctypeSystemKeyword);
                        std::get<DoctypeToken>(current_token_).system_identifier = "";
                        state_ = State::DoctypeSystemIdentifierDoubleQuoted;
                        continue;
                    case '\'':
                        emit(ParseError::MissingWhitespaceAfterDoctypeSystemKeyword);
                        std::get<DoctypeToken>(current_token_).system_identifier = "";
                        state_ = State::DoctypeSystemIdentifierSingleQuoted;
                        continue;
                    case '>':
                        emit(ParseError::MissingDoctypeSystemIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        emit(ParseError::MissingQuoteBeforeDoctypeSystemIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        reconsume_in(State::BogusDoctype);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#before-doctype-system-identifier-state
            case State::BeforeDoctypeSystemIdentifier: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        continue;
                    case '"':
                        std::get<DoctypeToken>(current_token_).system_identifier = "";
                        state_ = State::DoctypeSystemIdentifierDoubleQuoted;
                        continue;
                    case '\'':
                        std::get<DoctypeToken>(current_token_).system_identifier = "";
                        state_ = State::DoctypeSystemIdentifierSingleQuoted;
                        continue;
                    case '>':
                        emit(ParseError::MissingDoctypeSystemIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        emit(ParseError::MissingQuoteBeforeDoctypeSystemIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        reconsume_in(State::BogusDoctype);
                        continue;
                }
            }

            // Reachable via State::AfterDoctypePublicIdentifier,
            // State::BetweenDoctypePublicAndSystemIdentifiers,
            // State::AfterDoctypeSystemKeyword, and
            // State::BeforeDoctypeSystemIdentifier, all of which set
            // DoctypeToken::system_identifier to an empty string.
            // https://html.spec.whatwg.org/#doctype-system-identifier-(double-quoted)-state
            case State::DoctypeSystemIdentifierDoubleQuoted: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                // Help bugprone-unchecked-optional-access a bit.
                auto &doctype_system_identifier = std::get<DoctypeToken>(current_token_).system_identifier;
                assert(doctype_system_identifier);

                switch (*c) {
                    case '"':
                        state_ = State::AfterDoctypeSystemIdentifier;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        *doctype_system_identifier += kReplacementCharacter;
                        continue;
                    case '>':
                        emit(ParseError::AbruptDoctypeSystemIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        *doctype_system_identifier += *c;
                        continue;
                }
            }

            // Reachable via State::AfterDoctypePublicIdentifier,
            // State::BetweenDoctypePublicAndSystemIdentifiers,
            // State::AfterDoctypeSystemKeyword, and
            // State::BeforeDoctypeSystemIdentifier, all of which set
            // DoctypeToken::system_identifier to an empty string.
            // https://html.spec.whatwg.org/#doctype-system-identifier-(single-quoted)-state
            case State::DoctypeSystemIdentifierSingleQuoted: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                // Help bugprone-unchecked-optional-access a bit.
                auto &doctype_system_identifier = std::get<DoctypeToken>(current_token_).system_identifier;
                assert(doctype_system_identifier);

                switch (*c) {
                    case '\'':
                        state_ = State::AfterDoctypeSystemIdentifier;
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        *doctype_system_identifier += kReplacementCharacter;
                        continue;
                    case '>':
                        emit(ParseError::AbruptDoctypeSystemIdentifier);
                        std::get<DoctypeToken>(current_token_).force_quirks = true;
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        *doctype_system_identifier += *c;
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#after-doctype-system-identifier-state
            case State::AfterDoctypeSystemIdentifier: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInDoctype);
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        emit(ParseError::UnexpectedCharacterAfterDoctypeSystemIdentifier);
                        reconsume_in(State::BogusDoctype);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/#bogus-doctype-state
            case State::BogusDoctype: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        continue;
                    default:
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#cdata-section-state
            case State::CdataSection: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInCdata);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case ']':
                        state_ = State::CdataSectionBracket;
                        continue;
                    default:
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#cdata-section-bracket-state
            case State::CdataSectionBracket: {
                auto c = consume_next_input_character();
                if (c == ']') {
                    state_ = State::CdataSectionEnd;
                    continue;
                }

                emit(CharacterToken{']'});
                reconsume_in(State::CdataSection);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#cdata-section-end-state
            case State::CdataSectionEnd: {
                auto c = consume_next_input_character();
                if (c == ']') {
                    emit(CharacterToken{']'});
                    continue;
                }

                if (c == '>') {
                    state_ = State::Data;
                    continue;
                }

                emit(CharacterToken{']'});
                emit(CharacterToken{']'});
                reconsume_in(State::CdataSection);
                continue;
            }

            // https://html.spec.whatwg.org/#character-reference-state
            case State::CharacterReference: {
                temporary_buffer_ = "&"s;

                auto c = consume_next_input_character();
                if (!c) {
                    flush_code_points_consumed_as_a_character_reference();
                    reconsume_in(return_state_);
                    continue;
                }

                if (util::is_alphanumeric(*c)) {
                    reconsume_in(State::NamedCharacterReference);
                    continue;
                }

                switch (*c) {
                    case '#':
                        temporary_buffer_.append(1, *c);
                        state_ = State::NumericCharacterReference;
                        continue;
                    default:
                        flush_code_points_consumed_as_a_character_reference();
                        reconsume_in(return_state_);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#named-character-reference-state
            case State::NamedCharacterReference: {
                auto maybe_reference = find_named_character_reference_for(input_.substr(pos_ - 1));
                if (!maybe_reference) {
                    flush_code_points_consumed_as_a_character_reference();
                    state_ = State::AmbiguousAmpersand;
                    continue;
                }

                pos_ += maybe_reference->name.size() - 1;
                temporary_buffer_ = maybe_reference->name;

                auto c = peek_next_input_character();
                if (c.has_value() && consumed_as_part_of_an_attribute() && temporary_buffer_.back() != ';'
                        && (c == '=' || util::is_alphanumeric(*c))) {
                    flush_code_points_consumed_as_a_character_reference();
                    state_ = return_state_;
                    continue;
                }

                if (temporary_buffer_.back() != ';') {
                    emit(ParseError::MissingSemicolonAfterCharacterReference);
                }

                temporary_buffer_.clear();
                temporary_buffer_.append(unicode::to_utf8(maybe_reference->first_codepoint));
                if (maybe_reference->second_codepoint) {
                    temporary_buffer_.append(unicode::to_utf8(*maybe_reference->second_codepoint));
                }

                flush_code_points_consumed_as_a_character_reference();
                state_ = return_state_;
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#ambiguous-ampersand-state
            case State::AmbiguousAmpersand: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(return_state_);
                    continue;
                }

                if (util::is_alphanumeric(*c)) {
                    if (consumed_as_part_of_an_attribute()) {
                        current_attribute().value += *c;
                    } else {
                        emit(CharacterToken{*c});
                    }
                    continue;
                }

                if (*c == ';') {
                    emit(ParseError::UnknownNamedCharacterReference);
                }

                reconsume_in(return_state_);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#numeric-character-reference-state
            case State::NumericCharacterReference: {
                character_reference_code_ = 0;
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::DecimalCharacterReferenceStart);
                    continue;
                }

                switch (*c) {
                    case 'x':
                    case 'X':
                        temporary_buffer_ += *c;
                        state_ = State::HexadecimalCharacterReferenceStart;
                        continue;
                    default:
                        reconsume_in(State::DecimalCharacterReferenceStart);
                        continue;
                }
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#hexadecimal-character-reference-start-state
            case State::HexadecimalCharacterReferenceStart: {
                auto c = consume_next_input_character();
                if (c && util::is_hex_digit(*c)) {
                    reconsume_in(State::HexadecimalCharacterReference);
                    continue;
                }

                emit(ParseError::AbsenceOfDigitsInNumericCharacterReference);
                flush_code_points_consumed_as_a_character_reference();
                reconsume_in(return_state_);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#decimal-character-reference-start-state
            case State::DecimalCharacterReferenceStart: {
                auto c = consume_next_input_character();
                if (!c || !util::is_digit(*c)) {
                    emit(ParseError::AbsenceOfDigitsInNumericCharacterReference);
                    flush_code_points_consumed_as_a_character_reference();
                    reconsume_in(return_state_);
                    continue;
                }

                reconsume_in(State::DecimalCharacterReference);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#hexadecimal-character-reference-state
            case State::HexadecimalCharacterReference: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::MissingSemicolonAfterCharacterReference);
                    reconsume_in(State::NumericCharacterReferenceEnd);
                    continue;
                }

                if (util::is_digit(*c)) {
                    character_reference_code_ *= 16;
                    character_reference_code_ += *c - 0x30;
                    character_reference_code_ = std::min(character_reference_code_, 0x110000u);
                    continue;
                }

                if (util::is_upper_hex_digit(*c)) {
                    character_reference_code_ *= 16;
                    character_reference_code_ += *c - 0x37;
                    character_reference_code_ = std::min(character_reference_code_, 0x110000u);
                    continue;
                }

                if (util::is_lower_hex_digit(*c)) {
                    character_reference_code_ *= 16;
                    character_reference_code_ += *c - 0x57;
                    character_reference_code_ = std::min(character_reference_code_, 0x110000u);
                    continue;
                }

                if (c == ';') {
                    state_ = State::NumericCharacterReferenceEnd;
                    continue;
                }

                emit(ParseError::MissingSemicolonAfterCharacterReference);
                reconsume_in(State::NumericCharacterReferenceEnd);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#decimal-character-reference-state
            case State::DecimalCharacterReference: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::MissingSemicolonAfterCharacterReference);
                    reconsume_in(State::NumericCharacterReferenceEnd);
                    continue;
                }

                if (util::is_digit(*c)) {
                    character_reference_code_ *= 10;
                    character_reference_code_ += *c - 0x30;
                    character_reference_code_ = std::min(character_reference_code_, 0x110000u);
                    continue;
                }

                if (*c == ';') {
                    state_ = State::NumericCharacterReferenceEnd;
                    continue;
                }

                emit(ParseError::MissingSemicolonAfterCharacterReference);
                reconsume_in(State::NumericCharacterReferenceEnd);
                continue;
            }

            // https://html.spec.whatwg.org/multipage/parsing.html#numeric-character-reference-end-state
            case State::NumericCharacterReferenceEnd: {
                if (character_reference_code_ == 0) {
                    emit(ParseError::NullCharacterReference);
                    character_reference_code_ = 0xFFFD;
                }

                if (character_reference_code_ > 0x10FFFF) {
                    emit(ParseError::CharacterReferenceOutsideUnicodeRange);
                    character_reference_code_ = 0xFFFD;
                }

                if (unicode::is_surrogate(character_reference_code_)) {
                    emit(ParseError::SurrogateCharacterReference);
                    character_reference_code_ = 0xFFFD;
                }

                if (unicode::is_noncharacter(character_reference_code_)) {
                    emit(ParseError::NoncharacterCharacterReference);
                }

                if (character_reference_code_ == 0x0D
                        || (is_control(character_reference_code_) && !is_ascii_whitespace(character_reference_code_))) {
                    emit(ParseError::ControlCharacterReference);
                }

                static std::map<std::uint32_t, std::uint32_t> const replacements{{0x80, 0x20AC},
                        {0x82, 0x201A},
                        {0x83, 0x0192},
                        {0x84, 0x201E},
                        {0x85, 0x2026},
                        {0x86, 0x2020},
                        {0x87, 0x2021},
                        {0x88, 0x02C6},
                        {0x89, 0x2030},
                        {0x8A, 0x0160},
                        {0x8B, 0x2039},
                        {0x8C, 0x0152},
                        {0x8E, 0x017D},
                        {0x91, 0x2018},
                        {0x92, 0x2019},
                        {0x93, 0x201C},
                        {0x94, 0x201D},
                        {0x95, 0x2022},
                        {0x96, 0x2013},
                        {0x97, 0x2014},
                        {0x98, 0x02DC},
                        {0x99, 0x2122},
                        {0x9A, 0x0161},
                        {0x9B, 0x203A},
                        {0x9C, 0x0153},
                        {0x9E, 0x017E},
                        {0x9F, 0x0178}};

                if (auto it = replacements.find(character_reference_code_); it != replacements.end()) {
                    character_reference_code_ = it->second;
                }

                temporary_buffer_ = unicode::to_utf8(character_reference_code_);
                flush_code_points_consumed_as_a_character_reference();
                state_ = return_state_;
                continue;
            }
        }
    }
}

SourceLocation Tokenizer::current_source_location() const {
    int line = static_cast<int>(std::ranges::count(input_.substr(0, pos_), '\n')) + 1;
    auto col = input_.rfind('\n', pos_);
    return {.line = line, .column = static_cast<int>(line == 1 ? pos_ : pos_ - col - 1)};
}

void Tokenizer::emit(ParseError error) {
    on_error_(*this, error);
}

void Tokenizer::emit(Token &&token) {
    auto deduplicate = [](std::vector<Attribute> &attrs) {
        for (std::size_t i = 0; i < attrs.size(); ++i) {
            std::string_view name = attrs[i].name;
            for (std::size_t j = i + 1; j < attrs.size(); ++j) {
                if (attrs[j].name == name) {
                    attrs.erase(attrs.begin() + j);
                    j -= 1;
                }
            }
        }
    };

    if (auto *start_tag = std::get_if<StartTagToken>(&token)) {
        last_start_tag_name_ = start_tag->tag_name;
        deduplicate(start_tag->attributes);
    } else if (std::holds_alternative<EndTagToken>(token)) {
        // https://html.spec.whatwg.org/multipage/parsing.html#tokenization:parse-error-end-tag-with-attributes
        if (!end_tag_attributes_.empty()) {
            emit(ParseError::EndTagWithAttributes);
            end_tag_attributes_.clear();
        }

        // https://html.spec.whatwg.org/multipage/parsing.html#tokenization:parse-error-end-tag-with-trailing-solidus
        if (std::exchange(self_closing_end_tag_detected_, false)) {
            emit(ParseError::EndTagWithTrailingSolidus);
        }
    }

    on_emit_(*this, std::move(token));
}

std::optional<char> Tokenizer::consume_next_input_character() {
    if (is_eof()) {
        pos_ += 1;
        return std::nullopt;
    }

    return input_[pos_++];
}

std::optional<char> Tokenizer::peek_next_input_character() const {
    if (is_eof()) {
        return std::nullopt;
    }

    return input_[pos_];
}

bool Tokenizer::is_eof() const {
    return pos_ >= input_.size();
}

std::vector<Attribute> &Tokenizer::attributes_for_current_element() {
    if (auto *start_tag = std::get_if<StartTagToken>(&current_token_)) {
        return start_tag->attributes;
    }
    return end_tag_attributes_;
}

void Tokenizer::start_attribute_in_current_tag_token(Attribute attr) {
    attributes_for_current_element().push_back(std::move(attr));
}

Attribute &Tokenizer::current_attribute() {
    return attributes_for_current_element().back();
}

void Tokenizer::reconsume_in(State state) {
    --pos_;
    state_ = state;
}

bool Tokenizer::consumed_as_part_of_an_attribute() const {
    return return_state_ == State::AttributeValueDoubleQuoted || return_state_ == State::AttributeValueSingleQuoted
            || return_state_ == State::AttributeValueUnquoted;
}

void Tokenizer::flush_code_points_consumed_as_a_character_reference() {
    if (consumed_as_part_of_an_attribute()) {
        current_attribute().value += temporary_buffer_;
        return;
    }

    emit_temporary_buffer_as_character_tokens();
}

void Tokenizer::emit_temporary_buffer_as_character_tokens() {
    for (char c : temporary_buffer_) {
        emit(CharacterToken{c});
    }
}

bool Tokenizer::is_appropriate_end_tag_token(Token const &token) const {
    if (auto const *end_tag = std::get_if<EndTagToken>(&token)) {
        return end_tag->tag_name == last_start_tag_name_;
    }
    return false;
}

void Tokenizer::emit_replacement_character() {
    for (char c : kReplacementCharacter) {
        emit(CharacterToken{c});
    }
}

} // namespace html2
