// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_GET_H_
#define PROTOCOL_GET_H_

#include "protocol/handler_factory.h"
#include "protocol/response.h"
#include "uri/uri.h"

namespace protocol {

inline Response get(uri::Uri const &uri) {
    return HandlerFactory::create()->handle(uri);
}

} // namespace protocol

#endif
