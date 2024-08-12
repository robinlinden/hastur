// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_FILE_HANDLER_H_
#define PROTOCOL_FILE_HANDLER_H_

#include "protocol/iprotocol_handler.h"

#include "protocol/response.h"

#include "uri/uri.h"

#include <expected>

namespace protocol {

class FileHandler final : public IProtocolHandler {
public:
    [[nodiscard]] std::expected<Response, Error> handle(uri::Uri const &uri) override;
};

} // namespace protocol

#endif
