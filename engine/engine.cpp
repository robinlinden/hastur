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
#include "dom/xpath.h"
#include "html/parser.h"
#include "layout/layout.h"
#include "protocol/response.h"
#include "style/style.h"
#include "uri/uri.h"

#include <spdlog/spdlog.h>
#include <tl/expected.hpp>

#include <cstddef>
#include <future>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace engine {
namespace {

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Encoding#directives
[[nodiscard]] bool try_decompress_response_body(uri::Uri const &uri, protocol::Response &response) {
    auto encoding = response.headers.get("Content-Encoding");
    if (!encoding) {
        return true;
    }

    std::span<std::byte const> body_view{
            reinterpret_cast<std::byte const *>(response.body.data()), response.body.size()};

    if (encoding == "gzip" || encoding == "x-gzip" || encoding == "deflate") {
        auto zlib_mode = encoding == "deflate" ? archive::ZlibMode::Zlib : archive::ZlibMode::Gzip;
        auto decoded = archive::zlib_decode(body_view, zlib_mode);
        if (!decoded) {
            auto const &err = decoded.error();
            spdlog::error("Failed {}-decoding of '{}': '{}: {}'", *encoding, uri.uri, err.code, err.message);
            return false;
        }

        response.body.assign(reinterpret_cast<char const *>(decoded->data()), decoded->size());
        return true;
    }

    if (encoding == "zstd") {
        auto decoded = archive::zstd_decode(body_view);
        if (!decoded) {
            auto const &err = decoded.error();
            spdlog::error(
                    "Failed {}-decoding of '{}': '{}: {}'", *encoding, uri.uri, static_cast<int>(err), to_string(err));
            return false;
        }

        response.body.assign(reinterpret_cast<char const *>(decoded->data()), decoded->size());
        return true;
    }

    spdlog::warn("Got unsupported encoding '{}' from '{}'", *encoding, uri.uri);
    return false;
}

css::MediaQuery::Context to_media_context(Options opts) {
    return {
            .window_width = opts.layout_width,
            .color_scheme = opts.dark_mode ? css::ColorScheme::Dark : css::ColorScheme::Light,
    };
}

} // namespace

// NOLINTNEXTLINE(performance-unnecessary-value-param): Clang is wrong, the uri is moved below.
tl::expected<std::unique_ptr<PageState>, NavigationError> Engine::navigate(uri::Uri uri, Options opts) {
    auto result = load(std::move(uri));

    if (!result.response.has_value()) {
        return tl::unexpected{NavigationError{
                .uri = std::move(result.uri_after_redirects),
                .response = std::move(result.response.error()),
        }};
    }

    if (!try_decompress_response_body(result.uri_after_redirects, *result.response)) {
        return tl::unexpected{NavigationError{
                .uri = std::move(result.uri_after_redirects),
                .response{protocol::Error{
                        protocol::ErrorCode::InvalidResponse,
                        result.response->status_line,
                }},
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
        future_new_rules.push_back(std::async(std::launch::async, [this, link, &state]() -> css::StyleSheet {
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

            if (!try_decompress_response_body(*stylesheet_url, *style_data)) {
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
    state->viewport_height = opts.viewport_height;
    state->styled = style::style_tree(state->dom.html_node, state->stylesheet, to_media_context(opts));
    state->layout = layout::create_layout(*state->styled,
            {state->layout_width, state->viewport_height},
            *type_,
            get_intrensic_size_for_resource_at_url_);

    return state;
}

void Engine::relayout(PageState &state, Options opts) {
    state.layout_width = opts.layout_width;
    state.viewport_height = opts.viewport_height;
    state.styled = style::style_tree(state.dom.html_node, state.stylesheet, to_media_context(opts));
    state.layout = layout::create_layout(*state.styled,
            {state.layout_width, state.viewport_height},
            *type_,
            get_intrensic_size_for_resource_at_url_);
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
