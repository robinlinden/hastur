// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/iprotocol_handler.h"

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

    [[nodiscard]] Response handle(uri::Uri const &uri) override {
        if (!handlers_.contains(uri.scheme)) {
            return {Error::Unhandled};
        }

        return handlers_[uri.scheme]->handle(uri);
    }

private:
    std::map<std::string, std::unique_ptr<IProtocolHandler>, std::less<>> handlers_;
};

} // namespace protocol
