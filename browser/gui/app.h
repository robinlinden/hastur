// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef BROWSER_GUI_APP_H_
#define BROWSER_GUI_APP_H_

#include "dom/dom.h"
#include "layout/layout.h"
#include "protocol/get.h"
#include "style/style.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>

#include <optional>
#include <string>

namespace browser::gui {

class App final {
public:
    App(std::string browser_title, std::string start_page_hint, bool load_start_page);
    ~App();

    int run();

private:
    std::string browser_title_{};
    sf::RenderWindow window_{};
    sf::Clock clock_{};
    std::string url_buf_{};
    protocol::Response response_{};
    dom::Document dom_{};
    std::optional<style::StyledNode> styled_{};
    std::optional<layout::LayoutBox> layout_{};
    std::string status_line_str_{};
    std::string response_headers_str_{};
    std::string dom_str_{};
    std::string err_str_{};
    std::string layout_str_{};
    std::string mouse_over_str_{};

    // The scroll offset is the opposite of the current translation of the web page.
    // When we scroll "down", the web page is translated "up".
    float scroll_offset_y_{};

    void navigate();
    void layout();

    std::string get_hovered_element_text(layout::Position document_position) const;
    layout::Position to_document_position(layout::Position window_position) const;

    void reset_scroll();
    void scroll(float pixels);

    void run_overlay();
    void run_nav_widget();
    void run_http_response_widget() const;
    void run_dom_widget() const;
    void run_layout_widget() const;

    void clear_render_surface();
    void render_layout();
    void render_overlay();
    void show_render_surface();
};

} // namespace browser::gui

#endif
