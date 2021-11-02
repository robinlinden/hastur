// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/http.h"

#include <fmt/format.h>

#include <charconv>
#include <sstream>

using namespace std::string_view_literals;

namespace protocol {
namespace {

std::pair<std::string_view, std::string_view> split(std::string_view str, std::string_view sep) {
    if (auto it = str.find(sep); it != std::string::npos) {
        return {str.substr(0, it), str.substr(it + sep.size(), str.size() - it - sep.size())};
    }
    return {str, ""sv};
}

} // namespace

std::string to_string(std::map<std::string, std::string> const &headers) {
    std::stringstream ss{};
    for (auto const &header : headers) {
        ss << header.first << ": " << header.second << "\n";
    }
    return ss.str();
}

bool Http::use_port(uri::Uri const &uri) {
    if (uri.scheme == "http"sv) {
        if (!uri.authority.port.empty() && uri.authority.port != "80") {
            return true;
        }
    } else if (uri.scheme == "https"sv) {
        if (!uri.authority.port.empty() && uri.authority.port != "443") {
            return true;
        }
    }
    return false;
}

std::string Http::create_get_request(uri::Uri const &uri) {
    std::stringstream ss;
    ss << fmt::format("GET {} HTTP/1.1\r\n", uri.path);
    if (Http::use_port(uri)) {
        ss << fmt::format("Host: {}:{}\r\n", uri.authority.host, uri.authority.port);
    } else {
        ss << fmt::format("Host: {}\r\n", uri.authority.host);
    }
    ss << "Accept: text/html\r\n";
    ss << "Connection: close\r\n\r\n";

    return ss.str();
}

std::optional<StatusLine> Http::parse_status_line(std::string_view status_line) {
    auto sep1 = status_line.find(' ');
    if (sep1 == status_line.npos) {
        return std::nullopt;
    }

    auto sep2 = status_line.find(' ', sep1 + 1);
    if (sep2 == status_line.npos) {
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
            std::string{status_line.substr(sep2 + 1, status_line.size())},
    };
}

std::map<std::string, std::string> Http::parse_headers(std::string_view header) {
    std::map<std::string, std::string> headers{};
    for (auto sep = header.find("\r\n"); sep != std::string_view::npos; sep = header.find("\r\n")) {
        headers.emplace(split(header.substr(0, sep), ": "));
        header.remove_prefix(sep + 2);
    }
    headers.emplace(split(header, ": "));

    return headers;
}

} // namespace protocol
