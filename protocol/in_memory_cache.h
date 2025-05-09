// SPDX-FileCopyrightText: 2024-2025 Robin Lindén <dev@robinlinden.eu>
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
#include <mutex>
#include <utility>

namespace protocol {

// TODO(robinlinden): Eviction, invalidation, and partitioning.
class InMemoryCache : public IProtocolHandler {
public:
    explicit InMemoryCache(std::unique_ptr<IProtocolHandler> handler) : handler_{std::move(handler)} {}

    [[nodiscard]] tl::expected<Response, Error> handle(uri::Uri const &uri) override {
        {
            std::scoped_lock<std::mutex> lock{cache_mutex_};
            if (auto it = cache_.find(uri); it != cend(cache_)) {
                return it->second;
            }
        }

        auto response = handler_->handle(uri);
        std::scoped_lock<std::mutex> lock{cache_mutex_};
        return cache_[uri] = std::move(response);
    }

private:
    std::unique_ptr<IProtocolHandler> handler_;
    std::mutex cache_mutex_;
    std::map<uri::Uri, tl::expected<Response, Error>> cache_;
};

} // namespace protocol

#endif
