// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

#include "css/style_sheet.h"
#include "dom/dom.h"
#include "layout/layout_box.h"
#include "protocol/iprotocol_handler.h"
#include "protocol/response.h"
#include "style/styled_node.h"
#include "type/naive.h"
#include "type/type.h"
#include "uri/uri.h"

#include <tl/expected.hpp>

#include <memory>
#include <optional>
#include <utility>

namespace engine {

struct Options {
    // Default chosen by rolling 1d600.
    int layout_width{600};
    bool dark_mode{false};
};

struct PageState {
    uri::Uri uri{};
    protocol::Response response{};
    dom::Document dom{};
    css::StyleSheet stylesheet{};
    std::unique_ptr<style::StyledNode> styled{};
    std::optional<layout::LayoutBox> layout{};
    int layout_width{};
};

struct NavigationError {
    uri::Uri uri{};
    protocol::Error response{};
};

class Engine {
public:
    explicit Engine(std::unique_ptr<protocol::IProtocolHandler> protocol_handler,
            std::unique_ptr<type::IType> type = std::make_unique<type::NaiveType>())
        : protocol_handler_{std::move(protocol_handler)}, type_{std::move(type)} {}

    [[nodiscard]] tl::expected<std::unique_ptr<PageState>, NavigationError> navigate(uri::Uri, Options = {});

    void relayout(PageState &, Options);

    struct [[nodiscard]] LoadResult {
        std::expected<protocol::Response, protocol::Error> response;
        uri::Uri uri_after_redirects;
    };
    LoadResult load(uri::Uri);

    type::IType &font_system() { return *type_; }

private:
    std::unique_ptr<protocol::IProtocolHandler> protocol_handler_{};
    std::unique_ptr<type::IType> type_{};
};

} // namespace engine

#endif
