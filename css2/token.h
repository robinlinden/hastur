// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS2_TOKEN_H_
#define CSS2_TOKEN_H_

#include <cstdint>
#include <string>
#include <variant>

namespace css2 {

struct IdentToken {
    std::string data{};
    [[nodiscard]] bool operator==(IdentToken const &) const = default;
};

struct FunctionToken {
    std::string data{};
    [[nodiscard]] bool operator==(FunctionToken const &) const = default;
};

struct AtKeywordToken {
    std::string data{};
    [[nodiscard]] bool operator==(AtKeywordToken const &) const = default;
};

struct HashToken {
    enum class Type : std::uint8_t {
        Unrestricted,
        Id,
    };
    Type type{Type::Unrestricted};
    std::string data{};
    [[nodiscard]] bool operator==(HashToken const &) const = default;
};

struct StringToken {
    std::string data{};
    [[nodiscard]] bool operator==(StringToken const &) const = default;
};

struct BadStringToken {
    [[nodiscard]] bool operator==(BadStringToken const &) const = default;
};

struct UrlToken {
    std::string data{};
    [[nodiscard]] bool operator==(UrlToken const &) const = default;
};

struct BadUrlToken {
    [[nodiscard]] bool operator==(BadUrlToken const &) const = default;
};

struct DelimToken {
    char data{};
    [[nodiscard]] bool operator==(DelimToken const &) const = default;
};

struct NumberToken {
    std::variant<int, double> data;
    [[nodiscard]] bool operator==(NumberToken const &) const = default;

    [[nodiscard]] constexpr bool is_integer() const { return std::holds_alternative<int>(data); }
    [[nodiscard]] constexpr bool is_number() const { return !is_integer(); }
};

struct PercentageToken {
    std::variant<int, double> data{};
    [[nodiscard]] bool operator==(PercentageToken const &) const = default;

    [[nodiscard]] constexpr bool is_integer() const { return std::holds_alternative<int>(data); }
    [[nodiscard]] constexpr bool is_number() const { return !is_integer(); }
};

struct DimensionToken {
    std::variant<int, double> data{};
    std::string unit{};
    [[nodiscard]] bool operator==(DimensionToken const &) const = default;

    [[nodiscard]] constexpr bool is_integer() const { return std::holds_alternative<int>(data); }
    [[nodiscard]] constexpr bool is_number() const { return !is_integer(); }
};

struct WhitespaceToken {
    [[nodiscard]] bool operator==(WhitespaceToken const &) const = default;
};

struct CdoToken {
    [[nodiscard]] bool operator==(CdoToken const &) const = default;
};

struct CdcToken {
    [[nodiscard]] bool operator==(CdcToken const &) const = default;
};

struct ColonToken {
    [[nodiscard]] bool operator==(ColonToken const &) const = default;
};

struct SemiColonToken {
    [[nodiscard]] bool operator==(SemiColonToken const &) const = default;
};

struct CommaToken {
    [[nodiscard]] bool operator==(CommaToken const &) const = default;
};

struct OpenSquareToken {
    [[nodiscard]] bool operator==(OpenSquareToken const &) const = default;
};

struct CloseSquareToken {
    [[nodiscard]] bool operator==(CloseSquareToken const &) const = default;
};

struct OpenParenToken {
    [[nodiscard]] bool operator==(OpenParenToken const &) const = default;
};

struct CloseParenToken {
    [[nodiscard]] bool operator==(CloseParenToken const &) const = default;
};

struct OpenCurlyToken {
    [[nodiscard]] bool operator==(OpenCurlyToken const &) const = default;
};

struct CloseCurlyToken {
    [[nodiscard]] bool operator==(CloseCurlyToken const &) const = default;
};

using Token = std::variant<IdentToken,
        FunctionToken,
        AtKeywordToken,
        HashToken,
        StringToken,
        BadStringToken,
        UrlToken,
        BadUrlToken,
        DelimToken,
        NumberToken,
        PercentageToken,
        DimensionToken,
        WhitespaceToken,
        CdoToken,
        CdcToken,
        ColonToken,
        SemiColonToken,
        CommaToken,
        OpenSquareToken,
        CloseSquareToken,
        OpenParenToken,
        CloseParenToken,
        OpenCurlyToken,
        CloseCurlyToken>;

std::string to_string(Token const &);

} // namespace css2

#endif
