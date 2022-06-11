// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_IPROTOCOL_HANDLER_H_
#define PROTOCOL_IPROTOCOL_HANDLER_H_

#include "protocol/response.h"

#include "uri/uri.h"

namespace protocol {

class IProtocolHandler {
public:
    virtual ~IProtocolHandler() = default;
    [[nodiscard]] virtual Response handle(uri::Uri const &) = 0;
};

} // namespace protocol

#endif
