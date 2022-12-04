// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "engine/engine.h"

#include "css/default.h"
#include "css/parse.h"
#include "html/parse.h"
#include "style/style.h"

#include <spdlog/spdlog.h>

#include <future>
#include <iterator>
#include <string_view>
#include <utility>

using namespace std::literals;

namespace engine {
namespace {

std::optional<std::string_view> try_get_text_content(dom::Document const &doc, std::string_view path) {
    auto nodes = dom::nodes_by_path(doc.html(), path);
    if (nodes.empty() || nodes[0]->children.empty() || !std::holds_alternative<dom::Text>(nodes[0]->children[0])) {
        return std::nullopt;
    }
    return std::get<dom::Text>(nodes[0]->children[0]).text;
}

} // namespace

protocol::Error Engine::navigate(uri::Uri uri) {
    auto is_redirect = [](int status_code) {
        return status_code == 301 || status_code == 302 || status_code == 307 || status_code == 308;
    };

    uri_ = std::move(uri);
    response_ = protocol_handler_->handle(uri_);
    while (response_.err == protocol::Error::Ok && is_redirect(response_.status_line.status_code)) {
        spdlog::info("Following {} redirect from {} to {}",
                response_.status_line.status_code,
                uri_.uri,
                response_.headers.get("Location").value());
        uri_ = uri::Uri::parse(std::string(response_.headers.get("Location").value()), uri_);
        response_ = protocol_handler_->handle(uri_);
    }

    switch (response_.err) {
        case protocol::Error::Ok:
            on_navigation_success();
            break;
        default:
            on_navigation_failure_(response_.err);
            break;
    }

    return response_.err;
}

void Engine::set_layout_width(int width) {
    layout_width_ = width;
    if (!styled_) {
        return;
    }

    layout_ = layout::create_layout(*styled_, layout_width_);
    on_layout_update_();
}

void Engine::on_navigation_success() {
    dom_ = html::parse(response_.body);
    stylesheet_ = css::default_style();

    if (auto style = try_get_text_content(dom_, "html.head.style"sv)) {
        auto new_rules = css::parse(*style);
        stylesheet_.reserve(stylesheet_.size() + new_rules.size());
        stylesheet_.insert(
                end(stylesheet_), std::make_move_iterator(begin(new_rules)), std::make_move_iterator(end(new_rules)));
    }

    auto head_links = dom::nodes_by_path(dom_.html(), "html.head.link");
    std::erase_if(head_links, [](auto const *link) {
        return !link->attributes.contains("rel")
                || (link->attributes.contains("rel") && link->attributes.at("rel") != "stylesheet")
                || !link->attributes.contains("href");
    });

    // Start downloading all stylesheets.
    spdlog::info("Loading {} stylesheets", head_links.size());
    std::vector<std::future<std::vector<css::Rule>>> future_new_rules;
    for (auto link : head_links) {
        future_new_rules.push_back(std::async(std::launch::async, [=, this]() -> std::vector<css::Rule> {
            auto const &href = link->attributes.at("href");
            auto stylesheet_url = uri::Uri::parse(href, uri_);

            spdlog::info("Downloading stylesheet from {}", stylesheet_url.uri);
            auto style_data = protocol_handler_->handle(stylesheet_url);
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

            return css::parse(style_data.body);
        }));
    }

    // In order, wait for the download to finish and merge with the big stylesheet.
    for (auto &future_rules : future_new_rules) {
        auto rules = future_rules.get();
        stylesheet_.reserve(stylesheet_.size() + rules.size());
        stylesheet_.insert(
                end(stylesheet_), std::make_move_iterator(begin(rules)), std::make_move_iterator(end(rules)));
    }

    spdlog::info("Styling dom w/ {} rules", stylesheet_.size());
    styled_ = style::style_tree(dom_.html_node, stylesheet_);
    layout_ = layout::create_layout(*styled_, layout_width_);
    on_page_loaded_();
}

} // namespace engine
