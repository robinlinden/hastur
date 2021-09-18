// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/get.h"

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <fmt/format.h>

#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using namespace std::string_view_literals;

namespace protocol {
namespace {

std::pair<std::string_view, std::string_view> split(std::string_view str, std::string_view sep) {
    if (auto it = str.find(sep); it != std::string::npos) {
        return {str.substr(0, it), str.substr(it + sep.size(), str.size() - it - sep.size())};
    }
    return {str, ""sv};
}

std::optional<StatusLine> parse_status_line(std::string_view headers) {
    auto first_line_end = headers.find("\r\n");
    if (first_line_end == std::string_view::npos) {
        return std::nullopt;
    }

    headers.remove_suffix(headers.size() - first_line_end);
    auto sep1 = headers.find(' ');
    if (sep1 == std::string_view::npos) {
        return std::nullopt;
    }

    auto sep2 = headers.find(' ', sep1 + 1);
    if (sep2 == std::string_view::npos) {
        return std::nullopt;
    }

    int status_code = -1;
    auto status_str = headers.substr(sep1 + 1, sep1 + 4);
    std::from_chars(status_str.data(), status_str.data() + status_str.size(), status_code);
    if (status_code == -1) {
        return std::nullopt;
    }

    return StatusLine{
        std::string{headers.substr(0, sep1)},
        status_code,
        std::string{headers.substr(sep2 + 1, headers.size())},
    };
}

Response parse_response(std::string_view data) {
    auto [header, body] = split(data, "\r\n\r\n");
    auto status_line = parse_status_line(header);
    if (!status_line) {
        return {Error::InvalidResponse};
    }

    header.remove_prefix(header.find("\r\n") + 2);

    std::map<std::string, std::string> headers{};
    for (auto sep = header.find("\r\n"); sep != std::string_view::npos; sep = header.find("\r\n")) {
        headers.emplace(split(header.substr(0, sep), ": "));
        header.remove_prefix(sep + 2);
    }
    headers.emplace(split(header, ": "));

    return {
        Error::Ok,
        std::move(*status_line),
        std::move(headers),
        std::string{body},
    };
}

} // namespace

Response get(uri::Uri const &uri) {
    if (uri.scheme == "http"sv) {
        asio::ip::tcp::iostream stream(uri.authority.host, "http"sv);
        stream << fmt::format("GET {} HTTP/1.1\r\n", uri.path);
        stream << fmt::format("Host: {}\r\n", uri.authority.host);
        stream << "Accept: text/html\r\n";
        stream << "Connection: close\r\n\r\n";
        stream.flush();

        std::stringstream ss;
        ss << stream.rdbuf();
        std::string data{ss.str()};

        return parse_response(data);
    }

    if (uri.scheme == "https"sv) {
        asio::io_service svc;
        asio::ssl::context ctx{asio::ssl::context::method::sslv23_client};
        asio::ssl::stream<asio::ip::tcp::socket> ssock(svc, ctx);
        asio::error_code ec;

        asio::ip::tcp::resolver resolver{svc};
        auto endpoints = resolver.resolve(uri.authority.host, "https"sv, ec);
        if (ec) {
            return {Error::Unresolved};
        }

        ssock.lowest_layer().connect(*endpoints.begin());
        ssock.handshake(asio::ssl::stream_base::handshake_type::client);

        std::stringstream ss;
        ss << fmt::format("GET {} HTTP/1.1\r\n", uri.path);
        ss << fmt::format("Host: {}\r\n", uri.authority.host);
        ss << "Accept: text/html\r\n";
        ss << "Connection: close\r\n\r\n";
        asio::write(ssock, asio::buffer(ss.str()), ec);

        std::string data;
        while (true) {
            char buf[1024];
            std::size_t received = ssock.read_some(asio::buffer(buf), ec);
            if (ec) { break; }
            data.append(buf, buf + received);
        }

        return parse_response(data);
    }

    if (uri.scheme == "file"sv) {
        auto path = std::filesystem::path(uri.path);
        if (!exists(path)) {
            return {Error::Unresolved};
        }

        if (!is_regular_file(path)) {
            return {Error::InvalidResponse};
        }

        auto file = std::ifstream(path, std::ios::in | std::ios::binary);
        auto size = file_size(path);
        auto content = std::string(size, '\0');
        file.read(content.data(), size);
        return {Error::Ok, {}, {}, std::move(content)};
    }

    return {Error::Unhandled};
}

std::string to_string(std::map<std::string, std::string> const &headers) {
    std::stringstream ss{};
    for (auto const &header : headers) {
        ss << header.first << ": " << header.second << "\n";
    }
    return ss.str();
}

} // namespace protocol
