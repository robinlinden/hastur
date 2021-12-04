// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/engine.h"

#include "css/default.h"
#include "css/parse.h"
#include "html/parse.h"
#include "style/style.h"

#include <spdlog/spdlog.h>

#include <future>
#include <iterator>
#include <string_view>

using namespace std::literals;

namespace browser {
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

    if (uri.path.empty()) {
        uri.path = "/";
    }

    uri_ = uri;
    response_ = protocol::get(uri_);
    while (response_.err == protocol::Error::Ok && is_redirect(response_.status_line.status_code)) {
        spdlog::info("Following {} redirect from {} to {}",
                response_.status_line.status_code,
                uri_.uri,
                response_.headers.get("Location").value());
        uri_ = *uri::Uri::parse(std::string(response_.headers.get("Location").value()));
        if (uri_.path.empty()) {
            uri_.path = "/";
        }
        response_ = protocol::get(uri_);
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
    auto stylesheet{css::default_style()};

    if (auto style = try_get_text_content(dom_, "html.head.style"sv)) {
        auto new_rules = css::parse(*style);
        stylesheet.reserve(stylesheet.size() + new_rules.size());
        stylesheet.insert(
                end(stylesheet), std::make_move_iterator(begin(new_rules)), std::make_move_iterator(end(new_rules)));
    }

    auto head_links = dom::nodes_by_path(dom_.html(), "html.head.link");
    head_links.erase(std::remove_if(begin(head_links),
                             end(head_links),
                             [](auto const *link) {
                                 return link->attributes.contains("rel") && link->attributes.at("rel") != "stylesheet";
                             }),
            end(head_links));

    // Start downloading all stylesheets.
    spdlog::info("Loading {} stylesheets", head_links.size());
    std::vector<std::future<std::vector<css::Rule>>> future_new_rules;
    for (auto link : head_links) {
        future_new_rules.push_back(std::async(std::launch::async, [=, this] {
            auto stylesheet_url =
                    fmt::format("{}://{}{}", uri_.scheme, uri_.authority.host, link->attributes.at("href"));
            spdlog::info("Downloading stylesheet from {}", stylesheet_url);
            auto style_data = protocol::get(*uri::Uri::parse(stylesheet_url));
            return css::parse(style_data.body);
        }));
    }

    // In order, wait for the download to finish and merge with the big stylesheet.
    for (auto &future_rules : future_new_rules) {
        auto rules = future_rules.get();
        stylesheet.reserve(stylesheet.size() + rules.size());
        stylesheet.insert(end(stylesheet), std::make_move_iterator(begin(rules)), std::make_move_iterator(end(rules)));
    }

    spdlog::info("Styling dom w/ {} rules", stylesheet.size());
    styled_ = style::style_tree(dom_.html_node, stylesheet);
    layout_ = layout::create_layout(*styled_, layout_width_);
    on_page_loaded_();
}

} // namespace browser
