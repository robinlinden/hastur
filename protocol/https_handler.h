// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_HTTPS_HANDLER_H_
#define PROTOCOL_HTTPS_HANDLER_H_

#include "protocol/iprotocol_handler.h"

namespace protocol {

class HttpsHandler final : public IProtocolHandler {
public:
    [[nodiscard]] Response handle(uri::Uri const &) override;
};

} // namespace protocol

#endif
