// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/iprotocol_handler.h"

#include "protocol/response.h"

#include "uri/uri.h"

#include <tl/expected.hpp>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace protocol {

class MultiProtocolHandler final : public IProtocolHandler {
public:
    void add(std::string protocol, std::unique_ptr<IProtocolHandler> handler) {
        handlers_[std::move(protocol)] = std::move(handler);
    }

    [[nodiscard]] tl::expected<Response, Error> handle(uri::Uri const &uri) override {
        if (auto it = handlers_.find(uri.scheme); it != handlers_.end()) {
            return it->second->handle(uri);
        }

        return tl::unexpected{Error{ErrorCode::Unhandled}};
    }

private:
    std::map<std::string, std::unique_ptr<IProtocolHandler>, std::less<>> handlers_;
};

} // namespace protocol
