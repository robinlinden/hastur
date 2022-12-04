// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/handler_factory.h"

#include "protocol/file_handler.h"
#include "protocol/http_handler.h"
#include "protocol/https_handler.h"

#include <utility>

namespace protocol {

std::unique_ptr<MultiProtocolHandler> HandlerFactory::create(std::optional<std::string> user_agent) {
    auto handler = std::make_unique<MultiProtocolHandler>();
    handler->add("http", std::make_unique<HttpHandler>(user_agent));
    handler->add("https", std::make_unique<HttpsHandler>(std::move(user_agent)));
    handler->add("file", std::make_unique<FileHandler>());
    return handler;
}

} // namespace protocol
