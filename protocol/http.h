// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_HTTP_H_
#define PROTOCOL_HTTP_H_

#include "protocol/response.h"

#include "uri/uri.h"
#include "util/string.h"

#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace protocol {

class Http {
public:
    static Response get(auto &&socket, uri::Uri const &uri, std::optional<std::string_view> user_agent) {
        using namespace std::string_view_literals;

        if (!socket.connect(uri.authority.host, Http::use_port(uri) ? uri.authority.port : uri.scheme)) {
            return {ErrorCode::Unresolved};
        }

        socket.write(Http::create_get_request(uri, std::move(user_agent)));
        auto data = socket.read_until("\r\n"sv);
        if (data.empty()) {
            return {ErrorCode::InvalidResponse};
        }

        auto status_line = Http::parse_status_line(data.substr(0, data.size() - 2));
        if (!status_line) {
            return {ErrorCode::InvalidResponse};
        }

        data = socket.read_until("\r\n\r\n"sv);
        if (data.empty()) {
            return {ErrorCode::InvalidResponse, std::move(*status_line)};
        }

        auto headers = Http::parse_headers(data.substr(0, data.size() - 4));
        if (headers.size() == 0) {
            return {ErrorCode::InvalidResponse, std::move(*status_line)};
        }

        auto encoding = headers.get("transfer-encoding"sv);
        if (encoding == "chunked"sv) {
            auto body = Http::get_chunked_body(socket);
            if (!body) {
                return {ErrorCode::InvalidResponse, std::move(*status_line)};
            }

            data = *body;
        } else {
            data = socket.read_all();
        }

        return {ErrorCode::Ok, std::move(*status_line), std::move(headers), std::move(data)};
    }

private:
    static std::optional<std::string> get_chunked_body(auto &socket) {
        using namespace std::literals;

        std::string body{};
        while (true) {
            // Read first part of chunk
            std::string bytes = socket.read_until("\r\n"sv);
            bytes = util::trim(bytes);
            if (bytes.empty()) {
                break;
            }

            // TODO(mkiael): Handle chunk extensions

            // Decode chunk size
            std::size_t chunk_size{};
            auto result = std::from_chars(bytes.data(), bytes.data() + bytes.size(), chunk_size, 16);
            if (result.ec != std::errc()) {
                break;
            }

            // Check if this is the last chunk
            if (chunk_size == 0) {
                // TODO(mkiael): Handle trailer part
                socket.read_until("\r\n"sv);
                return body;
            }

            // Read chunk from socket
            bytes = socket.read_bytes(chunk_size);
            if (bytes.size() != chunk_size) {
                break;
            }

            // Append chunk to body
            body += bytes;

            // Read trailing \r\n before continuing with the next chunk
            bytes = socket.read_bytes(2);
            if (bytes != "\r\n"s) {
                break;
            }
        }
        return std::nullopt;
    }

    static bool use_port(uri::Uri const &uri);
    static std::string create_get_request(uri::Uri const &uri, std::optional<std::string_view> user_agent);
    static std::optional<StatusLine> parse_status_line(std::string_view status_line);
    static Headers parse_headers(std::string_view header);
};

} // namespace protocol

#endif
