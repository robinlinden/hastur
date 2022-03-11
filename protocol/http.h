// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_HTTP_H_
#define PROTOCOL_HTTP_H_

#include "uri/uri.h"
#include "util/string.h"

#include <charconv>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace protocol {

enum class Error {
    Ok,
    Unresolved,
    Unhandled,
    InvalidResponse,
};

struct StatusLine {
    std::string version;
    int status_code;
    std::string reason;
};

class Headers {
public:
    void add(std::pair<std::string_view, std::string_view> nv);
    [[nodiscard]] std::optional<std::string_view> get(std::string_view name) const;
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] std::size_t size() const;

private:
    struct CaseInsensitiveLess {
        using is_transparent = void;
        bool operator()(std::string_view s1, std::string_view s2) const;
    };
    std::map<std::string, std::string, CaseInsensitiveLess> headers_;
};

struct Response {
    Error err;
    StatusLine status_line;
    Headers headers;
    std::string body;
};

class Http {
public:
    static Response get(auto &&socket, uri::Uri const &uri) {
        using namespace std::string_view_literals;

        if (socket.connect(uri.authority.host, Http::use_port(uri) ? uri.authority.port : uri.scheme)) {
            socket.write(Http::create_get_request(uri));
            auto data = socket.read_until("\r\n"sv);
            if (data.empty()) {
                return {Error::Unresolved};
            }
            auto status_line = Http::parse_status_line(data.substr(0, data.size() - 2));
            if (!status_line) {
                return {Error::InvalidResponse};
            }
            data = socket.read_until("\r\n\r\n"sv);
            if (data.empty()) {
                return {Error::InvalidResponse};
            }
            auto headers = Http::parse_headers(data.substr(0, data.size() - 4));
            if (headers.size() == 0) {
                return {Error::InvalidResponse};
            }
            auto encoding = headers.get("transfer-encoding"sv);
            if (encoding == "chunked"sv) {
                if (auto body = Http::get_chunked_body(socket)) {
                    data = *body;
                } else {
                    return {Error::InvalidResponse};
                }
            } else {
                data = socket.read_all();
            }
            return {Error::Ok, std::move(*status_line), std::move(headers), std::move(data)};
        }

        return {Error::Unresolved};
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
    static std::string create_get_request(uri::Uri const &uri);
    static std::optional<StatusLine> parse_status_line(std::string_view status_line);
    static Headers parse_headers(std::string_view header);
};

} // namespace protocol

#endif
