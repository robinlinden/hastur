// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_RESPONSE_H_
#define PROTOCOL_RESPONSE_H_

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace protocol {
namespace detail {

struct CaseInsensitiveLess {
    using is_transparent = void;
    bool operator()(std::string_view, std::string_view) const;
};

} // namespace detail

enum class ErrorCode : std::uint8_t {
    Unresolved,
    Unhandled,
    InvalidResponse,
    RedirectLimit,
};

std::string_view to_string(ErrorCode);

struct StatusLine {
    std::string version;
    int status_code{};
    std::string reason;

    [[nodiscard]] bool operator==(StatusLine const &) const = default;
};

using Headers = std::multimap<std::string, std::string, detail::CaseInsensitiveLess>;

std::string to_string(Headers const &);

struct Response {
    StatusLine status_line;
    Headers headers;
    std::string body;

    [[nodiscard]] bool operator==(Response const &) const = default;
};

struct Error {
    ErrorCode err{};
    std::optional<StatusLine> status_line;

    [[nodiscard]] bool operator==(Error const &) const = default;
};

} // namespace protocol

#endif
