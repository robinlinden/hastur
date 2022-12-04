// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/https_handler.h"

#include "net/socket.h"
#include "protocol/http.h"

namespace protocol {

Response HttpsHandler::handle(uri::Uri const &uri) {
    return Http::get(net::SecureSocket{}, uri, user_agent_);
}

} // namespace protocol
