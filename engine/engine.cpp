// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "engine/engine.h"

#include "archive/zlib.h"
#include "css/default.h"
#include "css/parser.h"
#include "css/style_sheet.h"
#include "dom/dom.h"
#include "html/parser.h"
#include "layout/layout.h"
#include "protocol/response.h"
#include "style/style.h"
#include "uri/uri.h"

#include <spdlog/spdlog.h>
#include <tl/expected.hpp>

#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace engine {

tl::expected<std::unique_ptr<PageState>, NavigationError> Engine::navigate(uri::Uri uri) {
    auto result = load(std::move(uri));

    if (result.response.err != protocol::Error::Ok) {
        return tl::unexpected{NavigationError{
                .uri = std::move(result.uri_after_redirects),
                .response = std::move(result.response),
        }};
    }

    auto state = std::make_unique<PageState>();
    state->uri = std::move(result.uri_after_redirects);
    state->response = std::move(result.response);
    state->dom = html::parse(state->response.body);
    state->stylesheet = css::default_style();

    for (auto const &style : dom::nodes_by_xpath(state->dom.html(), "/html/head/style"sv)) {
        if (style->children.empty()) {
            continue;
        }

        // Style can only contain text, and we enforce this in our HTML parser.
        auto const &style_content = std::get<dom::Text>(style->children[0]);
        state->stylesheet.splice(css::parse(style_content.text));
    }

    auto head_links = dom::nodes_by_xpath(state->dom.html(), "/html/head/link");
    std::erase_if(head_links, [](auto const *link) {
        return !link->attributes.contains("rel")
                || (link->attributes.contains("rel") && link->attributes.at("rel") != "stylesheet")
                || !link->attributes.contains("href");
    });

    // Start downloading all stylesheets.
    spdlog::info("Loading {} stylesheets", head_links.size());
    std::vector<std::future<css::StyleSheet>> future_new_rules;
    future_new_rules.reserve(head_links.size());
    for (auto const *link : head_links) {
        future_new_rules.push_back(std::async(std::launch::async, [=, this, &state]() -> css::StyleSheet {
            auto const &href = link->attributes.at("href");
            auto stylesheet_url = uri::Uri::parse(href, state->uri);
            if (!stylesheet_url) {
                spdlog::warn("Failed to parse href '{}', skipping stylesheet", href);
                return {};
            }

            spdlog::info("Downloading stylesheet from {}", stylesheet_url->uri);
            auto res = load(*stylesheet_url);
            auto &style_data = res.response;
            stylesheet_url = std::move(res.uri_after_redirects);

            if (style_data.err != protocol::Error::Ok) {
                spdlog::warn("Error {} downloading {}", static_cast<int>(style_data.err), stylesheet_url->uri);
                return {};
            }

            if ((stylesheet_url->scheme == "http" || stylesheet_url->scheme == "https")
                    && style_data.status_line.status_code != 200) {
                spdlog::warn("Error {}: {} downloading {}",
                        style_data.status_line.status_code,
                        style_data.status_line.reason,
                        stylesheet_url->uri);
                return {};
            }

            // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Encoding#directives
            auto encoding = style_data.headers.get("Content-Encoding");
            if (encoding == "gzip" || encoding == "x-gzip" || encoding == "deflate") {
                auto zlib_mode = encoding == "deflate" ? archive::ZlibMode::Zlib : archive::ZlibMode::Gzip;
                auto decoded = archive::zlib_decode(style_data.body, zlib_mode);
                if (!decoded) {
                    auto const &err = decoded.error();
                    spdlog::error("Failed {}-decoding of '{}': '{}: {}'",
                            *encoding,
                            stylesheet_url->uri,
                            err.code,
                            err.message);
                    return {};
                }

                style_data.body = *std::move(decoded);
            } else if (encoding) {
                spdlog::warn("Got unsupported encoding '{}', skipping stylesheet '{}'", *encoding, stylesheet_url->uri);
                return {};
            }

            return css::parse(style_data.body);
        }));
    }

    // In order, wait for the download to finish and merge with the big stylesheet.
    for (auto &future_rules : future_new_rules) {
        state->stylesheet.splice(future_rules.get());
    }

    spdlog::info("Styling dom w/ {} rules", state->stylesheet.rules.size());
    state->layout_width = layout_width_;
    state->styled = style::style_tree(state->dom.html_node, state->stylesheet, {.window_width = state->layout_width});
    state->layout = layout::create_layout(*state->styled, state->layout_width, *type_);

    return state;
}

void Engine::relayout(PageState &state, int width) {
    state.layout_width = width;
    state.styled = style::style_tree(state.dom.html_node, state.stylesheet, {.window_width = state.layout_width});
    state.layout = layout::create_layout(*state.styled, state.layout_width, *type_);
}

Engine::LoadResult Engine::load(uri::Uri uri) {
    static constexpr int kMaxRedirects = 10;

    auto is_redirect = [](int status_code) {
        return status_code == 301 || status_code == 302 || status_code == 307 || status_code == 308;
    };

    int redirect_count = 0;
    protocol::Response response = protocol_handler_->handle(uri);
    while (response.err == protocol::Error::Ok && is_redirect(response.status_line.status_code)) {
        ++redirect_count;
        auto location = response.headers.get("Location");
        if (!location) {
            response.err = protocol::Error::InvalidResponse;
            return {std::move(response), std::move(uri)};
        }

        spdlog::info("Following {} redirect from {} to {}", response.status_line.status_code, uri.uri, *location);
        auto new_uri = uri::Uri::parse(std::string(*location), uri);
        if (!new_uri) {
            response.err = protocol::Error::InvalidResponse;
            return {std::move(response), std::move(uri)};
        }

        uri = *std::move(new_uri);
        response = protocol_handler_->handle(uri);
        if (redirect_count > kMaxRedirects) {
            response.err = protocol::Error::RedirectLimit;
            return {std::move(response), std::move(uri)};
        }
    }

    return {std::move(response), std::move(uri)};
}

} // namespace engine
