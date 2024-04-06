// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_RESPONSE_H_
#define PROTOCOL_RESPONSE_H_

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace protocol {

enum class Error : std::uint8_t {
    Ok,
    Unresolved,
    Unhandled,
    InvalidResponse,
    RedirectLimit,
};

std::string_view to_string(Error);

struct StatusLine {
    std::string version;
    int status_code{};
    std::string reason;

    [[nodiscard]] bool operator==(StatusLine const &) const = default;
};

class Headers {
public:
    Headers() = default;
    Headers(std::initializer_list<std::map<std::string, std::string>::value_type> init) : headers_{std::move(init)} {}

    void add(std::pair<std::string_view, std::string_view> nv);
    [[nodiscard]] std::optional<std::string_view> get(std::string_view name) const;
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] bool operator==(Headers const &) const = default;

private:
    struct CaseInsensitiveLess {
        using is_transparent = void;
        bool operator()(std::string_view s1, std::string_view s2) const;
    };
    std::map<std::string, std::string, CaseInsensitiveLess> headers_;
};

struct Response {
    Error err{};
    StatusLine status_line;
    Headers headers;
    std::string body;

    [[nodiscard]] bool operator==(Response const &) const = default;
};

} // namespace protocol

#endif
