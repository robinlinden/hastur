// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_HTTP_H_
#define PROTOCOL_HTTP_H_

#include "uri/uri.h"

#include <map>
#include <string>
#include <string_view>

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

struct Response {
    Error err;
    StatusLine status_line;
    std::map<std::string, std::string> headers;
    std::string body;
};

std::string to_string(std::map<std::string, std::string> const &headers);

class Http {
public:
    template<class SocketType>
    static Response get(SocketType &&socket, uri::Uri const &uri) {
        if (socket.connect(uri.authority.host, Http::use_port(uri) ? uri.authority.port : uri.scheme)) {
            socket.write(Http::create_get_request(uri));
            return Http::parse_response(socket.read_all());
        }

        return {Error::Unresolved};
    }

private:
    static bool use_port(uri::Uri const &uri);
    static std::string create_get_request(uri::Uri const &uri);
    static Response parse_response(std::string_view data);
};

} // namespace protocol

#endif
