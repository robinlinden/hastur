// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef BROWSER_GUI_ABOUT_HANDLER_H_
#define BROWSER_GUI_ABOUT_HANDLER_H_

#include "protocol/iprotocol_handler.h"
#include "protocol/response.h"
#include "uri/uri.h"

#include <tl/expected.hpp>

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

namespace browser::gui {

using Handlers = std::unordered_map<std::string, std::function<std::string()>>;

class AboutHandler : public protocol::IProtocolHandler {
public:
    explicit AboutHandler(Handlers pages) : pages_(std::move(pages)) {}

    [[nodiscard]] tl::expected<protocol::Response, protocol::Error> handle(uri::Uri const &uri) override {
        auto it = pages_.find(uri.path);
        if (it != pages_.end()) {
            return protocol::Response{{}, {}, it->second()};
        }

        return tl::unexpected{protocol::Error{protocol::ErrorCode::Unresolved}};
    }

private:
    Handlers pages_;
};

} // namespace browser::gui

#endif // BROWSER_GUI_ABOUT_HANDLER_H_
