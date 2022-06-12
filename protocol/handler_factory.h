// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_HANDLER_FACTORY_H_
#define PROTOCOL_HANDLER_FACTORY_H_

#include "protocol/multi_protocol_handler.h"

#include <memory>

namespace protocol {

class HandlerFactory {
public:
    [[nodiscard]] static std::unique_ptr<MultiProtocolHandler> create();
};

} // namespace protocol

#endif
