// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef BROWSER_GUI_APP_H_
#define BROWSER_GUI_APP_H_

#include "engine/engine.h"
#include "geom/geom.h"
#include "gfx/icanvas.h"
#include "layout/layout_box.h"
#include "protocol/response.h"
#include "uri/uri.h"
#include "util/history.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Cursor.hpp>
#include <tl/expected.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace browser::gui {

class App final {
public:
    App(std::string browser_title, std::string start_page_hint, bool load_start_page);
    ~App();

    void set_scale(unsigned scale);
    void step();
    int run();

private:
    engine::Engine engine_;
    tl::expected<std::unique_ptr<engine::PageState>, engine::NavigationError> maybe_page_{
            tl::unexpected<engine::NavigationError>{{}}};

    std::string browser_title_{};
    sf::Cursor cursor_{};
    sf::RenderWindow window_{};
    sf::Clock clock_{};
    std::string url_buf_{};
    std::string nav_widget_extra_info_{};

    enum class Canvas : std::uint8_t {
        OpenGL,
        Sfml,
    };

    Canvas selected_canvas_{Canvas::Sfml};
    std::unique_ptr<gfx::ICanvas> canvas_;

    // The scroll offset is the opposite of the current translation of the web page.
    // When we scroll "down", the web page is translated "up".
    int scroll_offset_y_{};

    bool render_debug_{};
    bool display_debug_gui_{};
    bool culling_enabled_{true};

    unsigned scale_{1};

    // ImGui needs a few iterations to settle.
    int process_iterations_{10};

    util::History<uri::Uri> browse_history_;

    engine::PageState &page() { return *maybe_page_.value(); }
    engine::PageState const &page() const { return *maybe_page_.value(); }

    void on_navigation_failure(protocol::ErrorCode);
    void on_page_loaded();
    void on_layout_updated();

    void navigate();
    void layout();

    void navigate_back();
    void navigate_forward();
    void reload();

    layout::LayoutBox const *get_hovered_node(geom::Position document_position) const;
    geom::Position to_document_position(geom::Position window_position) const;

    void reset_scroll();
    void scroll(int pixels);

    void run_overlay();
    void focus_url_input();
    void run_nav_widget();
    void run_debug_widget() const;

    void render_layout();
    void render_overlay();
    void show_render_surface();

    void switch_canvas();

    engine::Options make_options() const;
};

} // namespace browser::gui

#endif
