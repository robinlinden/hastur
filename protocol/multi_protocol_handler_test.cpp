// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/multi_protocol_handler.h"

#include "protocol/iprotocol_handler.h"
#include "protocol/response.h"

#include "etest/etest.h"
#include "uri/uri.h"

#include <memory>
#include <utility>

using etest::expect_eq;
using protocol::MultiProtocolHandler;

namespace {

class FakeProtocolHandler final : public protocol::IProtocolHandler {
public:
    explicit FakeProtocolHandler(protocol::Response response) : response_{std::move(response)} {}
    [[nodiscard]] protocol::Response handle(uri::Uri const &) override { return response_; }

private:
    protocol::Response response_;
};

} // namespace

int main() {
    etest::test("added protocols are handled", [] {
        MultiProtocolHandler handler;
        expect_eq(handler.handle(uri::Uri{.scheme = "hax"}).err, protocol::ErrorCode::Unhandled);

        handler.add("hax", std::make_unique<FakeProtocolHandler>(protocol::Response{protocol::ErrorCode::Ok}));
        expect_eq(handler.handle(uri::Uri{.scheme = "hax"}).err, protocol::ErrorCode::Ok);
    });

    return etest::run_all_tests();
}
