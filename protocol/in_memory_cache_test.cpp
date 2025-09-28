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
#include <future>
#include <memory>
#include <utility>
#include <vector>

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

    // The cache is used in a threaded context where we download things like
    // stylesheets and images in parallel. This threading will go away once
    // we've switched to async-io for downloading resources.
    s.add_test("thread safety", [](etest::IActions &a) {
        auto response = Response{.body{"hello"}};
        InMemoryCache cache{std::make_unique<FakeProtocolHandler>([&] { return response; })};
        uri::Uri const uri;

        std::vector<std::future<Response>> jobs;
        jobs.reserve(2);
        jobs.push_back(std::async(std::launch::async, [&cache, &uri] { return cache.handle(uri).value(); }));
        jobs.push_back(std::async(std::launch::async, [&cache, &uri] { return cache.handle(uri).value(); }));

        for (auto &job : jobs) {
            a.expect_eq(job.get(), response);
        }
    });

    s.add_test("no-store responses are not cached", [](etest::IActions &a) {
        int calls{};
        InMemoryCache cache{std::make_unique<FakeProtocolHandler>([&] {
            ++calls;
            return Response{
                    .headers{{"Cache-Control", "no-store"}},
                    .body{"hello"},
            };
        })};

        a.expect_eq(calls, 0);
        a.expect_eq(cache.handle({}).value().body, "hello");
        a.expect_eq(calls, 1);
        a.expect_eq(cache.handle({}).value().body, "hello");
        a.expect_eq(calls, 2);
    });

    return s.run();
}
