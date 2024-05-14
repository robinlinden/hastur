// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_IPROTOCOL_HANDLER_H_
#define PROTOCOL_IPROTOCOL_HANDLER_H_

#include "protocol/response.h"

#include "uri/uri.h"

#include <expected>

namespace protocol {

class IProtocolHandler {
public:
    virtual ~IProtocolHandler() = default;
    [[nodiscard]] virtual std::expected<Response, Error> handle(uri::Uri const &) = 0;
};

} // namespace protocol

#endif
