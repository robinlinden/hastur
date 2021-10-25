// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_GET_H_
#define PROTOCOL_GET_H_

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

Response get(uri::Uri const &uri);

Response parse(std::string_view data);

std::string to_string(std::map<std::string, std::string> const &headers);

} // namespace protocol

#endif
