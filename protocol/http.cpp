// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/http.h"

#include "protocol/response.h"

#include "uri/uri.h"
#include "util/string.h"

#include <charconv>
#include <format>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using namespace std::string_view_literals;

namespace protocol {
namespace {
constexpr bool is_valid_header(std::pair<std::string_view, std::string_view> const &header) {
    return !header.first.empty() && !header.second.empty();
}
} // namespace

bool Http::use_port(uri::Uri const &uri) {
    if (uri.scheme == "http"sv) {
        return !uri.authority.port.empty() && uri.authority.port != "80";
    }

    if (uri.scheme == "https"sv) {
        return !uri.authority.port.empty() && uri.authority.port != "443";
    }

    return true;
}

std::string Http::create_get_request(uri::Uri const &uri, std::optional<std::string_view> user_agent) {
    std::stringstream ss;
    ss << std::format("GET {}", uri.path);
    if (!uri.query.empty()) {
        ss << '?' << uri.query;
    }

    ss << " HTTP/1.1\r\n";

    if (Http::use_port(uri)) {
        ss << std::format("Host: {}:{}\r\n", uri.authority.host, uri.authority.port);
    } else {
        ss << std::format("Host: {}\r\n", uri.authority.host);
    }
    ss << "Accept: text/html\r\n";
    ss << "Connection: close\r\n";
    if (user_agent) {
        ss << std::format("User-Agent: {}\r\n", *user_agent);
    }

    ss << "\r\n";

    return std::move(ss).str();
}

// https://datatracker.ietf.org/doc/html/rfc9112#section-4
std::optional<StatusLine> Http::parse_status_line(std::string_view status_line) {
    auto sep1 = status_line.find(' ');
    if (sep1 == std::string_view::npos) {
        return std::nullopt;
    }

    auto sep2 = status_line.find(' ', sep1 + 1);
    if (sep2 == std::string_view::npos) {
        return std::nullopt;
    }

    int status_code = -1;
    auto status_str = status_line.substr(sep1 + 1, sep1 + 4);
    std::from_chars(status_str.data(), status_str.data() + status_str.size(), status_code);
    if (status_code == -1) {
        return std::nullopt;
    }

    return StatusLine{
            std::string{status_line.substr(0, sep1)},
            status_code,
            std::string{status_line.substr(sep2 + 1)},
    };
}

// https://datatracker.ietf.org/doc/html/rfc9112#section-5
Headers Http::parse_headers(std::string_view header) {
    Headers headers;
    for (auto sep = header.find("\r\n"); sep != std::string_view::npos; sep = header.find("\r\n")) {
        auto kv = util::split_once(header.substr(0, sep), ':');
        if (is_valid_header(kv)) {
            kv.second = util::trim(kv.second);
            headers.add(std::move(kv));
        }

        header.remove_prefix(sep + 2);
    }

    auto kv = util::split_once(header, ':');
    if (is_valid_header(kv)) {
        kv.second = util::trim(kv.second);
        headers.add(std::move(kv));
    }

    return headers;
}

} // namespace protocol
