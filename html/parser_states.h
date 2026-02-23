// SPDX-FileCopyrightText: 2023-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSER_STATES_H_
#define HTML_PARSER_STATES_H_

#include "html/token.h"

#include <optional>
#include <variant>
#include <vector>

namespace html {

class IActions;

struct Initial;
struct BeforeHtml;
struct BeforeHead;
struct InHead;
struct InHeadNoscript;
struct AfterHead;
struct InBody;
struct Text;
struct InTable;
struct InTableText;
struct InCaption;
struct InColumnGroup;
struct InTableBody;
struct InRow;
struct InCell;
struct InSelect;
struct InSelectInTable;
struct InTemplate;
struct AfterBody;
struct InFrameset;
struct AfterFrameset;
struct AfterAfterBody;
struct AfterAfterFrameset;

using InsertionMode = std::variant<Initial,
        BeforeHtml,
        BeforeHead,
        InHead,
        InHeadNoscript,
        AfterHead,
        InBody,
        Text,
        InTable,
        InTableText,
        // InCaption,
        // InColumnGroup,
        InTableBody,
        InRow,
        InCell,
        // InSelect,
        // InSelectInTable,
        // InTemplate,
        AfterBody,
        InFrameset,
        AfterFrameset,
        AfterAfterBody
        // AfterAfterFrameset
        >;

struct InCaption {};
struct InColumnGroup {};
struct InSelect {};
struct InSelectInTable {};
struct InTemplate {};
struct AfterAfterFrameset {};

[[nodiscard]] InsertionMode appropriate_insertion_mode(IActions &);

struct Initial {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

struct BeforeHtml {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

struct BeforeHead {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

struct InHead {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

struct InHeadNoscript {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

struct AfterHead {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

struct InBody {
    std::optional<InsertionMode> process(IActions &, Token const &);
    bool ignore_next_lf{};
};

struct Text {
    std::optional<InsertionMode> process(IActions &, Token const &);
    bool ignore_next_lf{};
};

struct InTable {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

struct InTableText {
    std::optional<InsertionMode> process(IActions &, Token const &);
    std::vector<CharacterToken> pending_character_tokens;
};

struct InTableBody {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

struct InRow {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

struct InCell {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

struct AfterBody {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

struct InFrameset {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

struct AfterFrameset {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

struct AfterAfterBody {
    std::optional<InsertionMode> process(IActions &, Token const &);
};

} // namespace html

#endif
