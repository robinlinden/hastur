// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML2_PARSER_STATES_H_
#define HTML2_PARSER_STATES_H_

#include "html2/tokenizer.h"

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
        Text //
#if 0
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
        AfterAfterFrameset
#endif
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
struct InFrameset {};
struct AfterFrameset {};
struct AfterAfterBody {};
struct AfterAfterFrameset {};

// https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
// Incomplete.
struct Initial {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

// https://html.spec.whatwg.org/multipage/parsing.html#the-before-html-insertion-mode
// Incomplete.
struct BeforeHtml {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

// https://html.spec.whatwg.org/multipage/parsing.html#the-before-head-insertion-mode
struct BeforeHead {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead
// Incomplete.
struct InHead {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inheadnoscript
struct InHeadNoscript {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

// https://html.spec.whatwg.org/multipage/parsing.html#the-after-head-insertion-mode
// Incomplete.
struct AfterHead {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
// Incomplete.
struct InBody {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-incdata
// Incomplete.
struct Text {
    std::optional<InsertionMode> process(IActions &, html2::Token const &);
};

} // namespace html2

#endif
