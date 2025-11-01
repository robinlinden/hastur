// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_TOKEN_H_
#define HTML_TOKEN_H_

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace html {

struct DoctypeToken {
    std::optional<std::string> name{std::nullopt};
    std::optional<std::string> public_identifier{std::nullopt};
    std::optional<std::string> system_identifier{std::nullopt};
    bool force_quirks{false};
    [[nodiscard]] bool operator==(DoctypeToken const &) const = default;
};

struct Attribute {
    std::string name;
    std::string value;
    [[nodiscard]] bool operator==(Attribute const &) const = default;
};

struct StartTagToken {
    std::string tag_name;
    bool self_closing{false};
    std::vector<Attribute> attributes;
    [[nodiscard]] bool operator==(StartTagToken const &) const = default;
};

struct EndTagToken {
    std::string tag_name;
    [[nodiscard]] bool operator==(EndTagToken const &) const = default;
};

struct CommentToken {
    std::string data;
    [[nodiscard]] bool operator==(CommentToken const &) const = default;
};

struct CharacterToken {
    char data{};
    [[nodiscard]] bool operator==(CharacterToken const &) const = default;
};

struct EndOfFileToken {
    [[nodiscard]] bool operator==(EndOfFileToken const &) const = default;
};

using Token = std::variant<DoctypeToken, StartTagToken, EndTagToken, CommentToken, CharacterToken, EndOfFileToken>;

std::string to_string(Token const &);

} // namespace html

#endif
