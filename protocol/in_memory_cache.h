// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_IN_MEMORY_CACHE_H_
#define PROTOCOL_IN_MEMORY_CACHE_H_

#include "protocol/iprotocol_handler.h"
#include "protocol/response.h"

#include "uri/uri.h"

#include <tl/expected.hpp>

#include <map>
#include <memory>
#include <utility>

namespace protocol {

// TODO(robinlinden): Eviction, invalidation, and partitioning.
class InMemoryCache : public IProtocolHandler {
public:
    explicit InMemoryCache(std::unique_ptr<IProtocolHandler> handler) : handler_{std::move(handler)} {}

    [[nodiscard]] tl::expected<Response, Error> handle(uri::Uri const &uri) override {
        if (auto it = cache_.find(uri); it != cend(cache_)) {
            return it->second;
        }

        return cache_[uri] = handler_->handle(uri);
    }

private:
    std::unique_ptr<IProtocolHandler> handler_;
    std::map<uri::Uri, tl::expected<Response, Error>> cache_;
};

} // namespace protocol

#endif
