// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_FILE_HANDLER_H_
#define PROTOCOL_FILE_HANDLER_H_

#include "protocol/iprotocol_handler.h"

namespace protocol {

class FileHandler final : public IProtocolHandler {
public:
    [[nodiscard]] Response handle(uri::Uri const &uri) override;
};

} // namespace protocol

#endif
