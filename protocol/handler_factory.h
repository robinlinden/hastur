// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_HANDLER_FACTORY_H_
#define PROTOCOL_HANDLER_FACTORY_H_

#include "protocol/multi_protocol_handler.h"

#include <memory>
#include <optional>
#include <string>

namespace protocol {

class HandlerFactory {
public:
    [[nodiscard]] static std::unique_ptr<MultiProtocolHandler> create(
            std::optional<std::string> user_agent = std::nullopt);
};

} // namespace protocol

#endif
