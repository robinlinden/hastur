// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML2_PARSER_STATES_H_
#define HTML2_PARSER_STATES_H_

#include "html2/token.h"

#include <optional>
#include <variant>

namespace html2 {

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
        // InTable,
        // InTableText,
        // InCaption,
        // InColumnGroup,
        // InTableBody,
        // InRow,
        // InCell,
        // InSelect,
        // InSelectInTable,
        // InTemplate,
        // AfterBody,
        InFrameset,
        AfterFrameset
        // AfterAfterBody,
        // AfterAfterFrameset
        >;

struct InTable {};
struct InTableText {};
struct InCaption {};
struct InColumnGroup {};
struct InTableBody {};
struct InRow {};
struct InCell {};
struct InSelect {};
struct InSelectInTable {};
struct InTemplate {};
struct AfterBody {};
struct AfterAfterBody {};
struct AfterAfterFrameset {};

struct Initial {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

struct BeforeHtml {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

struct BeforeHead {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

struct InHead {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

struct InHeadNoscript {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

struct AfterHead {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

struct InBody {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

struct Text {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

struct InFrameset {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

struct AfterFrameset {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

} // namespace html2

#endif
