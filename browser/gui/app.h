// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef BROWSER_GUI_APP_H_
#define BROWSER_GUI_APP_H_

#include "dom/dom.h"
#include "engine/engine.h"
#include "gfx/icanvas.h"
#include "gfx/sfml_canvas.h"
#include "layout/layout_box.h"
#include "protocol/handler_factory.h"
#include "uri/uri.h"
#include "util/history.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Cursor.hpp>

#include <memory>
#include <string>
#include <vector>

namespace browser::gui {

class App final {
public:
    App(std::string browser_title, std::string start_page_hint, bool load_start_page);
    ~App();

    void set_scale(unsigned scale);
    int run();

private:
    // Latest Firefox ESR user agent (on Windows). This matches what the Tor browser does.
    engine::Engine engine_{protocol::HandlerFactory::create(
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:102.0) Gecko/20100101 Firefox/102.0")};
    bool page_loaded_{};

    std::string browser_title_{};
    sf::Cursor cursor_{};
    sf::RenderWindow window_{};
    sf::Clock clock_{};
    std::string url_buf_{};
    std::string status_line_str_{};
    std::string response_headers_str_{};
    std::string dom_str_{};
    std::string stylesheet_str_{};
    std::string layout_str_{};
    std::string nav_widget_extra_info_{};

    enum class Canvas {
        OpenGL,
        Sfml,
    };

    Canvas selected_canvas_{Canvas::Sfml};
    std::unique_ptr<gfx::ICanvas> canvas_{std::make_unique<gfx::SfmlCanvas>(window_)};

    // The scroll offset is the opposite of the current translation of the web page.
    // When we scroll "down", the web page is translated "up".
    int scroll_offset_y_{};

    bool render_debug_{};
    bool display_debug_gui_{};

    unsigned scale_{1};

    // ImGui needs a few iterations to settle.
    int process_iterations_{10};

    util::History<uri::Uri> browse_history_;

    void on_navigation_failure(protocol::Error);
    void on_page_loaded();
    void on_layout_updated();

    void navigate();
    void layout();

    layout::LayoutBox const *get_hovered_node(geom::Position document_position) const;
    geom::Position to_document_position(geom::Position window_position) const;

    void reset_scroll();
    void scroll(int pixels);

    void update_status_line();

    void run_overlay();
    void focus_url_input();
    void run_nav_widget();
    void run_http_response_widget() const;
    void run_dom_widget() const;
    void run_stylesheet_widget() const;
    void run_layout_widget() const;

    void clear_render_surface();
    void render_layout();
    void render_overlay();
    void show_render_surface();

    void switch_canvas();
};

} // namespace browser::gui

#endif
