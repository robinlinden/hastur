// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef BROWSER_ENGINE_H_
#define BROWSER_ENGINE_H_

#include "dom/dom.h"
#include "layout/layout.h"
#include "protocol/iprotocol_handler.h"
#include "style/styled_node.h"
#include "uri/uri.h"

#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace browser {

class Engine {
public:
    explicit Engine(std::unique_ptr<protocol::IProtocolHandler> protocol_handler)
        : protocol_handler_{std::move(protocol_handler)} {}

    protocol::Error navigate(uri::Uri uri);

    void set_layout_width(int width);

    void set_on_navigation_failure(auto cb) { on_navigation_failure_ = std::move(cb); }
    void set_on_page_loaded(auto cb) { on_page_loaded_ = std::move(cb); }
    void set_on_layout_updated(auto cb) { on_layout_update_ = std::move(cb); }

    uri::Uri const &uri() const { return uri_; }
    protocol::Response const &response() const { return response_; }
    dom::Document const &dom() const { return dom_; }
    layout::LayoutBox const &layout() const { return *layout_; }

private:
    std::function<void(protocol::Error)> on_navigation_failure_{[](protocol::Error) {
    }};
    std::function<void()> on_page_loaded_{[] {
    }};
    std::function<void()> on_layout_update_{[] {
    }};

    int layout_width_{};

    std::unique_ptr<protocol::IProtocolHandler> protocol_handler_{};

    uri::Uri uri_{};
    protocol::Response response_{};
    dom::Document dom_{};
    std::unique_ptr<style::StyledNode> styled_{};
    std::optional<layout::LayoutBox> layout_{};

    void on_navigation_success();
};

} // namespace browser

#endif
