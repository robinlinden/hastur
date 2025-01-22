// SPDX-FileCopyrightText: 2024-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/in_memory_cache.h"

#include "protocol/iprotocol_handler.h"
#include "protocol/response.h"

#include "etest/etest2.h"
#include "uri/uri.h"

#include <tl/expected.hpp>

#include <functional>
#include <memory>
#include <utility>

using namespace protocol;

namespace {

class FakeProtocolHandler final : public protocol::IProtocolHandler {
public:
    explicit FakeProtocolHandler(std::function<Response()> on_handle) : on_handle_{std::move(on_handle)} {}
    tl::expected<protocol::Response, protocol::Error> handle(uri::Uri const &) override { return on_handle_(); }

private:
    std::function<Response()> on_handle_;
};

} // namespace

int main() {
    etest::Suite s{};

    s.add_test("cache returns cached response", [](etest::IActions &a) {
        int calls{};
        auto response = Response{.body{"hello"}};
        InMemoryCache cache{std::make_unique<FakeProtocolHandler>([&] {
            ++calls;
            return response;
        })};

        uri::Uri const uri;
        a.expect_eq(calls, 0);
        a.expect_eq(cache.handle(uri), response);
        a.expect_eq(calls, 1);
        a.expect_eq(cache.handle(uri), response);
        a.expect_eq(calls, 1);
    });

    return s.run();
}
