// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/http_handler.h"

#include "net/socket.h"
#include "protocol/http.h"
#include "protocol/response.h"
#include "uri/uri.h"

#include <tl/expected.hpp>

namespace protocol {

tl::expected<Response, Error> HttpHandler::handle(uri::Uri const &uri) {
    return Http::get(net::Socket{}, uri, user_agent_);
}

} // namespace protocol
