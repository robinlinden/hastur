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
#include "protocol/response.h"
#include "style/styled_node.h"
#include "type/naive.h"
#include "type/type.h"
#include "uri/uri.h"

#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace engine {

class Engine {
public:
    explicit Engine(std::unique_ptr<protocol::IProtocolHandler> protocol_handler,
            std::unique_ptr<type::IType const> type = std::make_unique<type::NaiveType>())
        : protocol_handler_{std::move(protocol_handler)}, type_{std::move(type)} {}

    protocol::Error navigate(uri::Uri uri);

    void set_layout_width(int width);

    void set_on_navigation_failure(std::function<void(protocol::Error)> cb) { on_navigation_failure_ = std::move(cb); }
    void set_on_page_loaded(std::function<void()> cb) { on_page_loaded_ = std::move(cb); }
    void set_on_layout_updated(std::function<void()> cb) { on_layout_update_ = std::move(cb); }

    uri::Uri const &uri() const { return uri_; }
    protocol::Response const &response() const { return response_; }
    dom::Document const &dom() const { return dom_; }
    css::StyleSheet const &stylesheet() const { return stylesheet_; }
    layout::LayoutBox const *layout() const { return layout_.has_value() ? &*layout_ : nullptr; }

    struct [[nodiscard]] LoadResult {
        protocol::Response response;
        uri::Uri uri_after_redirects;
    };
    LoadResult load(uri::Uri);

private:
    std::function<void(protocol::Error)> on_navigation_failure_{[](protocol::Error) {
    }};
    std::function<void()> on_page_loaded_{[] {
    }};
    std::function<void()> on_layout_update_{[] {
    }};

    int layout_width_{};

    std::unique_ptr<protocol::IProtocolHandler> protocol_handler_{};
    std::unique_ptr<type::IType const> type_{};

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
