// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/multi_protocol_handler.h"

#include "protocol/iprotocol_handler.h"
#include "protocol/response.h"

#include "etest/etest.h"
#include "uri/uri.h"

#include <expected>
#include <memory>
#include <utility>

using etest::expect_eq;
using protocol::MultiProtocolHandler;

namespace {

class FakeProtocolHandler final : public protocol::IProtocolHandler {
public:
    explicit FakeProtocolHandler(protocol::Response response) : response_{std::move(response)} {}
    [[nodiscard]] std::expected<protocol::Response, protocol::Error> handle(uri::Uri const &) override {
        return response_;
    }

private:
    std::expected<protocol::Response, protocol::Error> response_;
};

} // namespace

int main() {
    etest::test("added protocols are handled", [] {
        MultiProtocolHandler handler;
        expect_eq(handler.handle(uri::Uri{.scheme = "hax"}),
                std::unexpected{protocol::Error{protocol::ErrorCode::Unhandled}});

        handler.add("hax", std::make_unique<FakeProtocolHandler>(protocol::Response{}));
        expect_eq(handler.handle(uri::Uri{.scheme = "hax"}), protocol::Response{});
    });

    return etest::run_all_tests();
}
