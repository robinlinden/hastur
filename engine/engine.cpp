// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "engine/engine.h"

#include "archive/zlib.h"
#include "archive/zstd.h"
#include "css/default.h"
#include "css/media_query.h"
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

#include <cstdint>
#include <future>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::literals;

namespace engine {
namespace {

css::MediaQuery::Context to_media_context(Options opts) {
    return {
            .window_width = opts.layout_width,
            .color_scheme = opts.dark_mode ? css::ColorScheme::Dark : css::ColorScheme::Light,
    };
}

} // namespace

tl::expected<std::unique_ptr<PageState>, NavigationError> Engine::navigate(uri::Uri uri, Options opts) {
    auto result = load(std::move(uri));

    if (!result.response.has_value()) {
        return tl::unexpected{NavigationError{
                .uri = std::move(result.uri_after_redirects),
                .response = std::move(result.response.error()),
        }};
    }

    auto state = std::make_unique<PageState>();
    state->uri = std::move(result.uri_after_redirects);
    state->response = std::move(result.response.value());
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

            if (!style_data.has_value()) {
                spdlog::warn("Error {} downloading {}", static_cast<int>(style_data.error().err), stylesheet_url->uri);
                return {};
            }

            if ((stylesheet_url->scheme == "http" || stylesheet_url->scheme == "https")
                    && style_data->status_line.status_code != 200) {
                spdlog::warn("Error {}: {} downloading {}",
                        style_data->status_line.status_code,
                        style_data->status_line.reason,
                        stylesheet_url->uri);
                return {};
            }

            // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Encoding#directives
            auto encoding = style_data->headers.get("Content-Encoding");
            if (encoding == "gzip" || encoding == "x-gzip" || encoding == "deflate") {
                auto zlib_mode = encoding == "deflate" ? archive::ZlibMode::Zlib : archive::ZlibMode::Gzip;
                auto decoded = archive::zlib_decode(style_data->body, zlib_mode);
                if (!decoded) {
                    auto const &err = decoded.error();
                    spdlog::error("Failed {}-decoding of '{}': '{}: {}'",
                            *encoding,
                            stylesheet_url->uri,
                            err.code,
                            err.message);
                    return {};
                }

                style_data->body = *std::move(decoded);
            } else if (encoding == "zstd") {
                static_assert(std::is_same_v<char, std::uint8_t> || std::is_same_v<unsigned char, std::uint8_t>);
                std::span<std::uint8_t const> body_view{
                        reinterpret_cast<std::uint8_t const *>(style_data->body.data()), style_data->body.size()};
                auto decoded = archive::zstd_decode(body_view);
                if (!decoded) {
                    auto const &err = decoded.error();
                    spdlog::error("Failed {}-decoding of '{}': '{}: {}'",
                            *encoding,
                            stylesheet_url->uri,
                            static_cast<int>(err),
                            to_string(err));
                    return {};
                }

                style_data->body = std::string(reinterpret_cast<char const *>(decoded->data()), decoded->size());
            } else if (encoding) {
                spdlog::warn("Got unsupported encoding '{}', skipping stylesheet '{}'", *encoding, stylesheet_url->uri);
                return {};
            }

            return css::parse(style_data->body);
        }));
    }

    // In order, wait for the download to finish and merge with the big stylesheet.
    for (auto &future_rules : future_new_rules) {
        state->stylesheet.splice(future_rules.get());
    }

    spdlog::info("Styling dom w/ {} rules", state->stylesheet.rules.size());
    state->layout_width = opts.layout_width;
    state->styled = style::style_tree(state->dom.html_node, state->stylesheet, to_media_context(opts));
    state->layout = layout::create_layout(*state->styled, state->layout_width, *type_);

    return state;
}

void Engine::relayout(PageState &state, Options opts) {
    state.layout_width = opts.layout_width;
    state.styled = style::style_tree(state.dom.html_node, state.stylesheet, to_media_context(opts));
    state.layout = layout::create_layout(*state.styled, state.layout_width, *type_);
}

Engine::LoadResult Engine::load(uri::Uri uri) {
    static constexpr int kMaxRedirects = 10;

    auto is_redirect = [](int status_code) {
        return status_code == 301 || status_code == 302 || status_code == 307 || status_code == 308;
    };

    int redirect_count = 0;
    auto response = protocol_handler_->handle(uri);
    while (response.has_value() && is_redirect(response->status_line.status_code)) {
        ++redirect_count;
        auto location = response->headers.get("Location");
        if (!location) {
            return {
                    .response = tl::unexpected{protocol::Error{
                            protocol::ErrorCode::InvalidResponse, std::move(response->status_line)}},
                    .uri_after_redirects = std::move(uri),
            };
        }

        spdlog::info("Following {} redirect from {} to {}", response->status_line.status_code, uri.uri, *location);
        auto new_uri = uri::Uri::parse(std::string(*location), uri);
        if (!new_uri) {
            return {
                    .response = tl::unexpected{protocol::Error{
                            protocol::ErrorCode::InvalidResponse, std::move(response->status_line)}},
                    .uri_after_redirects = std::move(uri),
            };
        }

        uri = *std::move(new_uri);
        response = protocol_handler_->handle(uri);
        if (redirect_count > kMaxRedirects) {
            return {
                    .response = tl::unexpected{protocol::Error{
                            protocol::ErrorCode::RedirectLimit, std::move(response->status_line)}},
                    .uri_after_redirects = std::move(uri),
            };
        }
    }

    return {std::move(response), std::move(uri)};
}

} // namespace engine
