// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

#include "css/style_sheet.h"
#include "dom/dom.h"
#include "layout/layout.h"
#include "protocol/iprotocol_handler.h"
#include "style/styled_node.h"
#include "uri/uri.h"

#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace engine {

class Engine {
public:
    explicit Engine(std::unique_ptr<protocol::IProtocolHandler> protocol_handler)
        : protocol_handler_{std::move(protocol_handler)} {}

    protocol::Error navigate(uri::Uri uri);

    void set_layout_width(int width);
    void set_whitespace_mode(layout::WhitespaceMode);
    layout::WhitespaceMode whitespace_mode() const { return whitespace_mode_; }

    void set_on_navigation_failure(auto cb) { on_navigation_failure_ = std::move(cb); }
    void set_on_page_loaded(auto cb) { on_page_loaded_ = std::move(cb); }
    void set_on_layout_updated(auto cb) { on_layout_update_ = std::move(cb); }

    uri::Uri const &uri() const { return uri_; }
    protocol::Response const &response() const { return response_; }
    dom::Document const &dom() const { return dom_; }
    css::StyleSheet const &stylesheet() const { return stylesheet_; }
    layout::LayoutBox const *layout() const { return layout_.has_value() ? &*layout_ : nullptr; }

private:
    std::function<void(protocol::Error)> on_navigation_failure_{[](protocol::Error) {
    }};
    std::function<void()> on_page_loaded_{[] {
    }};
    std::function<void()> on_layout_update_{[] {
    }};

    int layout_width_{};
    layout::WhitespaceMode whitespace_mode_{layout::WhitespaceMode::Collapse};

    std::unique_ptr<protocol::IProtocolHandler> protocol_handler_{};

    uri::Uri uri_{};
    protocol::Response response_{};
    dom::Document dom_{};
    css::StyleSheet stylesheet_{};
    std::unique_ptr<style::StyledNode> styled_{};
    std::optional<layout::LayoutBox> layout_{};

    void on_navigation_success();
};

} // namespace engine

#endif
