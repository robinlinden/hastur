// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/handler_factory.h"

#include "protocol/file_handler.h"
#include "protocol/http_handler.h"
#include "protocol/https_handler.h"

namespace protocol {

std::unique_ptr<MultiProtocolHandler> HandlerFactory::create() {
    auto handler = std::make_unique<MultiProtocolHandler>();
    handler->add("http", std::make_unique<HttpHandler>());
    handler->add("https", std::make_unique<HttpsHandler>());
    handler->add("file", std::make_unique<FileHandler>());
    return handler;
}

} // namespace protocol
