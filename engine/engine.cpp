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

#include <future>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace engine {

protocol::Error Engine::navigate(uri::Uri uri) {
    auto result = load(std::move(uri));
    state_.response = std::move(result.response);
    state_.uri = std::move(result.uri_after_redirects);

    switch (state_.response.err) {
        case protocol::Error::Ok:
            on_navigation_success();
            break;
        default:
            on_navigation_failure_(state_.response.err);
            break;
    }

    return state_.response.err;
}

void Engine::set_layout_width(int width) {
    state_.layout_width = width;
    if (!state_.styled) {
        return;
    }

    state_.styled = style::style_tree(state_.dom.html_node, state_.stylesheet, {.window_width = state_.layout_width});
    state_.layout = layout::create_layout(*state_.styled, state_.layout_width, *type_);
    on_layout_update_();
}

void Engine::on_navigation_success() {
    state_.dom = html::parse(state_.response.body);
    state_.stylesheet = css::default_style();

    for (auto const &style : dom::nodes_by_xpath(state_.dom.html(), "/html/head/style"sv)) {
        if (style->children.empty()) {
            continue;
        }

        // Style can only contain text, and we enforce this in our HTML parser.
        auto const &style_content = std::get<dom::Text>(style->children[0]);
        state_.stylesheet.splice(css::parse(style_content.text));
    }

    auto head_links = dom::nodes_by_xpath(state_.dom.html(), "/html/head/link");
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
        future_new_rules.push_back(std::async(std::launch::async, [=, this]() -> css::StyleSheet {
            auto const &href = link->attributes.at("href");
            auto stylesheet_url = uri::Uri::parse(href, state_.uri);

            spdlog::info("Downloading stylesheet from {}", stylesheet_url.uri);
            auto res = load(stylesheet_url);
            auto &style_data = res.response;
            stylesheet_url = std::move(res.uri_after_redirects);

            if (style_data.err != protocol::Error::Ok) {
                spdlog::warn("Error {} downloading {}", static_cast<int>(style_data.err), stylesheet_url.uri);
                return {};
            }

            if ((stylesheet_url.scheme == "http" || stylesheet_url.scheme == "https")
                    && style_data.status_line.status_code != 200) {
                spdlog::warn("Error {}: {} downloading {}",
                        style_data.status_line.status_code,
                        style_data.status_line.reason,
                        stylesheet_url.uri);
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
                            stylesheet_url.uri,
                            err.code,
                            err.message);
                    return {};
                }

                style_data.body = *std::move(decoded);
            } else if (encoding) {
                spdlog::warn("Got unsupported encoding '{}', skipping stylesheet '{}'", *encoding, stylesheet_url.uri);
                return {};
            }

            return css::parse(style_data.body);
        }));
    }

    // In order, wait for the download to finish and merge with the big stylesheet.
    for (auto &future_rules : future_new_rules) {
        state_.stylesheet.splice(future_rules.get());
    }

    spdlog::info("Styling dom w/ {} rules", state_.stylesheet.rules.size());
    state_.styled = style::style_tree(state_.dom.html_node, state_.stylesheet, {.window_width = state_.layout_width});
    state_.layout = layout::create_layout(*state_.styled, state_.layout_width, *type_);
    on_page_loaded_();
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
        uri = uri::Uri::parse(std::string(*location), uri);
        response = protocol_handler_->handle(uri);
        if (redirect_count > kMaxRedirects) {
            response.err = protocol::Error::RedirectLimit;
            return {std::move(response), std::move(uri)};
        }
    }

    return {std::move(response), std::move(uri)};
}

} // namespace engine
