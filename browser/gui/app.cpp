// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/gui/app.h"

#include "css/rule.h"
#include "dom/dom.h"
#include "gfx/color.h"
#include "gfx/opengl_canvas.h"
#include "render/render.h"
#include "uri/uri.h"

#include <SFML/Window/Event.hpp>
#include <fmt/format.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>
#include <variant>

using namespace std::literals;

namespace browser::gui {
namespace {

auto constexpr kDefaultResolutionX = 1024;
auto constexpr kDefaultResolutionY = 768;

// Magic number that felt right during testing.
auto constexpr kMouseWheelScrollFactor = 10;

std::optional<std::string_view> try_get_text_content(dom::Document const &doc, std::string_view xpath) {
    auto nodes = dom::nodes_by_xpath(doc.html(), xpath);
    if (nodes.empty() || nodes[0]->children.empty()) {
        return std::nullopt;
    }

    if (auto const *text = std::get_if<dom::Text>(&nodes[0]->children[0])) {
        return text->text;
    }

    return std::nullopt;
}

void ensure_has_scheme(std::string &url) {
    if (!url.contains("://")) {
        spdlog::info("Url missing scheme, assuming https");
        url = fmt::format("https://{}", url);
    }
}

std::optional<std::string_view> try_get_uri(layout::LayoutBox const *from) {
    if (from == nullptr) {
        return std::nullopt;
    }

    for (auto const *node = from->node; node != nullptr; node = node->parent) {
        auto const *element = std::get_if<dom::Element>(&node->node);
        if (element && element->name == "a"sv && element->attributes.contains("href")) {
            return element->attributes.at("href");
        }
    }

    return std::nullopt;
}

std::string element_text(layout::LayoutBox const *element) {
    if (element == nullptr || element->node == nullptr) {
        return ""s;
    }

    // Special handling of <a> because I want to see what link I'm hovering.
    if (auto uri = try_get_uri(element); uri.has_value()) {
        return "a: "s + std::string{*uri};
    }

    auto const &dom_node = element->node->node;
    if (auto const *text = std::get_if<dom::Text>(&dom_node)) {
        return text->text;
    }

    return std::get<dom::Element>(dom_node).name;
}

std::string stylesheet_to_string(std::vector<css::Rule> const &stylesheet) {
    std::stringstream ss;
    for (auto const &rule : stylesheet) {
        ss << css::to_string(rule) << std::endl;
    }
    return std::move(ss).str();
}

namespace im {
// TODO(robinlinden): Stronger types.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void window(char const *title, ImVec2 const &position, ImVec2 const &size, auto content) {
    ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
    ImGui::Begin(title, nullptr, ImGuiWindowFlags_HorizontalScrollbar);
    content();
    ImGui::End();
}
} // namespace im
} // namespace

App::App(std::string browser_title, std::string start_page_hint, bool load_start_page)
    : browser_title_{std::move(browser_title)}, window_{sf::VideoMode(kDefaultResolutionX, kDefaultResolutionY),
                                                        browser_title_},
      url_buf_{std::move(start_page_hint)} {
    window_.setMouseCursor(cursor_);
    if (!ImGui::SFML::Init(window_)) {
        spdlog::critical("imgui-sfml initialization failed");
        std::abort();
    }

    // This is okay as long as we don't call e.g. setenv(), unsetenv(), or putenv().
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    if (std::getenv("HST_DISABLE_DISK_IO")) {
        // TODO(robinlinden): Support for things like HST_DISABLE_DISK_IO=0 to
        // re-enable IO.
        ImGui::GetIO().IniFilename = nullptr;
    }

    canvas_->set_viewport_size(window_.getSize().x, window_.getSize().y);

    engine_.set_layout_width(window_.getSize().x / scale_);
    engine_.set_on_navigation_failure(std::bind_front(&App::on_navigation_failure, this));
    engine_.set_on_page_loaded(std::bind_front(&App::on_page_loaded, this));
    engine_.set_on_layout_updated(std::bind_front(&App::on_layout_updated, this));

    if (load_start_page) {
        ensure_has_scheme(url_buf_);
        navigate();
    }
}

App::~App() {
    ImGui::SFML::Shutdown();
}

void App::set_scale(unsigned scale) {
    scale_ = scale;
    ImGui::GetIO().FontGlobalScale = static_cast<float>(scale_);
    canvas_->set_scale(scale_);
    auto window_size = window_.getSize();

    // Only resize the window if the user hasn't resized it.
    if (window_size.x == kDefaultResolutionX && window_size.y == kDefaultResolutionY) {
        window_.setSize({kDefaultResolutionX * scale_, kDefaultResolutionY * scale_});
        canvas_->set_viewport_size(window_.getSize().x, window_.getSize().y);
    }

    engine_.set_layout_width(window_size.x / scale_);
}

int App::run() {
    while (window_.isOpen()) {
        sf::Event event{};
        while (window_.pollEvent(event)) {
            // ImGui needs a few iterations to do what it wants to do. This was
            // pretty much picked at random after I still occasionally got
            // unexpected results when giving it 2 iterations.
            process_iterations_ = 5;
            ImGui::SFML::ProcessEvent(event);

            switch (event.type) {
                case sf::Event::Closed: {
                    window_.close();
                    break;
                }
                case sf::Event::Resized: {
                    canvas_->set_viewport_size(event.size.width, event.size.height);
                    engine_.set_layout_width(event.size.width / scale_);
                    break;
                }
                case sf::Event::KeyPressed: {
                    if (ImGui::GetIO().WantCaptureKeyboard) {
                        break;
                    }

                    switch (event.key.code) {
                        case sf::Keyboard::Key::J: {
                            scroll(event.key.shift ? -20 : -5);
                            break;
                        }
                        case sf::Keyboard::Key::K: {
                            scroll(event.key.shift ? 20 : 5);
                            break;
                        }
                        case sf::Keyboard::Key::F1: {
                            render_debug_ = !render_debug_;
                            break;
                        }
                        case sf::Keyboard::Key::F2: {
                            switch_canvas();
                            break;
                        }
                        case sf::Keyboard::Key::F3: {
                            auto mode = engine_.whitespace_mode();
                            engine_.set_whitespace_mode(mode == layout::WhitespaceMode::Preserve
                                            ? layout::WhitespaceMode::Collapse
                                            : layout::WhitespaceMode::Preserve);
                            break;
                        }
                        case sf::Keyboard::Key::F4: {
                            display_debug_gui_ = !display_debug_gui_;
                            break;
                        }
                        case sf::Keyboard::Key::Left: {
                            if (!event.key.alt) {
                                break;
                            }

                            auto entry = browse_history_.previous();
                            if (!entry) {
                                break;
                            }

                            browse_history_.pop();
                            url_buf_ = entry->uri;
                            navigate();
                            break;
                        }
                        case sf::Keyboard::Key::Right: {
                            if (!event.key.alt) {
                                break;
                            }

                            auto entry = browse_history_.next();
                            if (!entry) {
                                break;
                            }

                            url_buf_ = entry->uri;
                            navigate();
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
                case sf::Event::MouseMoved: {
                    if (!page_loaded_) {
                        break;
                    }

                    auto window_position = geom::Position{event.mouseMove.x, event.mouseMove.y};
                    auto document_position = to_document_position(std::move(window_position));
                    auto const *hovered = get_hovered_node(document_position);
                    nav_widget_extra_info_ =
                            fmt::format("{},{}: {}", document_position.x, document_position.y, element_text(hovered));

                    // If imgui is dealing with the mouse, we do nothing and let imgui change the cursor.
                    if (ImGui::GetIO().WantCaptureMouse) {
                        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
                        break;
                    }

                    // Otherwise we tell imgui not to mess with the cursor, and change it according to what we're
                    // currently hovering over.
                    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
                    if (try_get_uri(hovered).has_value()) {
                        cursor_.loadFromSystem(sf::Cursor::Hand);
                    } else {
                        cursor_.loadFromSystem(sf::Cursor::Arrow);
                    }
                    window_.setMouseCursor(cursor_);

                    break;
                }
                case sf::Event::MouseButtonReleased: {
                    if (ImGui::GetIO().WantCaptureMouse || event.mouseButton.button != sf::Mouse::Left) {
                        break;
                    }

                    auto window_position = geom::Position{event.mouseButton.x, event.mouseButton.y};
                    auto document_position = to_document_position(std::move(window_position));
                    auto const *hovered = get_hovered_node(std::move(document_position));
                    if (auto uri = try_get_uri(hovered); uri.has_value()) {
                        url_buf_ = std::string{*uri};
                        navigate();
                    }

                    break;
                }
                case sf::Event::MouseWheelScrolled: {
                    if (ImGui::GetIO().WantCaptureMouse
                            || event.mouseWheelScroll.wheel != sf::Mouse::Wheel::VerticalWheel) {
                        break;
                    }

                    scroll(std::lround(event.mouseWheelScroll.delta) * kMouseWheelScrollFactor);
                    break;
                }
                default:
                    break;
            }
        }

        if (process_iterations_ == 0) {
            // The sleep duration was picked at random.
            std::this_thread::sleep_for(std::chrono::milliseconds{5});
            continue;
        }
        process_iterations_ -= 1;

        run_overlay();
        run_nav_widget();
        if (display_debug_gui_) {
            run_http_response_widget();
            run_dom_widget();
            run_stylesheet_widget();
            run_layout_widget();
        }

        clear_render_surface();

        if (page_loaded_) {
            render_layout();
        }

        render_overlay();
        show_render_surface();
    }

    return 0;
}

void App::navigate() {
    page_loaded_ = false;
    auto uri = uri::Uri::parse(url_buf_, engine_.uri());
    browse_history_.push(uri);
    engine_.navigate(std::move(uri));

    // Make sure the displayed url is still correct if we followed any redirects.
    url_buf_ = engine_.uri().uri;
}

void App::on_navigation_failure(protocol::Error err) {
    update_status_line();
    response_headers_str_ = engine_.response().headers.to_string();
    dom_str_.clear();
    stylesheet_str_.clear();
    layout_str_.clear();

    switch (err) {
        case protocol::Error::Unresolved: {
            nav_widget_extra_info_ = fmt::format("Unable to resolve endpoint for '{}'", url_buf_);
            spdlog::error(nav_widget_extra_info_);
            break;
        }
        case protocol::Error::Unhandled: {
            nav_widget_extra_info_ = fmt::format("Unhandled protocol for '{}'", url_buf_);
            spdlog::error(nav_widget_extra_info_);
            break;
        }
        case protocol::Error::InvalidResponse: {
            nav_widget_extra_info_ = fmt::format("Invalid response from '{}'", url_buf_);
            spdlog::error(nav_widget_extra_info_);
            break;
        }
        case protocol::Error::Ok:
        default:
            spdlog::error("This should never happen: {}", static_cast<int>(err));
            break;
    }
}

void App::on_page_loaded() {
    page_loaded_ = true;
    if (auto page_title = try_get_text_content(engine_.dom(), "/html/head/title"sv)) {
        window_.setTitle(fmt::format("{} - {}", *page_title, browser_title_));
    } else {
        window_.setTitle(browser_title_);
    }

    update_status_line();
    response_headers_str_ = engine_.response().headers.to_string();
    dom_str_ = dom::to_string(engine_.dom());
    stylesheet_str_ = stylesheet_to_string(engine_.stylesheet());
    on_layout_updated();
}

void App::on_layout_updated() {
    reset_scroll();
    nav_widget_extra_info_.clear();
    auto const *layout = engine_.layout();
    layout_str_ = layout != nullptr ? layout::to_string(*layout) : "";
}

layout::LayoutBox const *App::get_hovered_node(geom::Position document_position) const {
    auto const *layout = engine_.layout();
    if (!page_loaded_ || layout == nullptr) {
        return nullptr;
    }

    return layout::box_at_position(*layout, document_position);
}

geom::Position App::to_document_position(geom::Position window_position) const {
    return {window_position.x / static_cast<int>(scale_),
            window_position.y / static_cast<int>(scale_) - scroll_offset_y_};
}

void App::reset_scroll() {
    canvas_->add_translation(0, -scroll_offset_y_);
    scroll_offset_y_ = 0;
}

void App::scroll(int pixels) {
    auto const *layout = engine_.layout();
    if (!page_loaded_ || layout == nullptr) {
        return;
    }

    // Don't allow scrolling if the entire page fits on the screen.
    if (static_cast<int>(window_.getSize().y) > layout->dimensions.margin_box().height) {
        return;
    }

    // Don't allow overscroll in either direction.
    if (scroll_offset_y_ + pixels > 0) {
        pixels = -scroll_offset_y_;
    }

    int current_bottom_visible_y = static_cast<int>(window_.getSize().y) - scroll_offset_y_;
    int scrolled_bottom_visible_y = current_bottom_visible_y - pixels;
    if (scrolled_bottom_visible_y > layout->dimensions.margin_box().height) {
        pixels -= layout->dimensions.margin_box().height - scrolled_bottom_visible_y;
    }

    canvas_->add_translation(0, pixels);
    scroll_offset_y_ += pixels;
}

void App::update_status_line() {
    auto const &r = engine_.response();
    status_line_str_ = fmt::format("{} {} {}", r.status_line.version, r.status_line.status_code, r.status_line.reason);
}

void App::run_overlay() {
    ImGui::SFML::Update(window_, clock_.restart());
}

void App::run_nav_widget() {
    im::window("Navigation", {0, 0}, {window_.getSize().x / 2.f, 0}, [this] {
        if (ImGui::InputText("Url", &url_buf_, ImGuiInputTextFlags_EnterReturnsTrue)) {
            ensure_has_scheme(url_buf_);
            navigate();
        }
        ImGui::TextUnformatted(nav_widget_extra_info_.c_str());
    });
}

void App::run_http_response_widget() const {
    auto const &size = window_.getSize();
    im::window("HTTP Response", {size.x / 2.f, 0}, {size.x / 2.f, size.y / 2.f}, [this] {
        ImGui::TextUnformatted(status_line_str_.c_str());
        if (ImGui::CollapsingHeader("Headers")) {
            ImGui::TextUnformatted(response_headers_str_.c_str());
        }
        if (ImGui::CollapsingHeader("Body")) {
            ImGui::TextUnformatted(engine_.response().body.c_str());
        }
    });
}

void App::run_dom_widget() const {
    im::window("DOM", {0, 70.f * scale_}, {window_.getSize().x / 2.f, window_.getSize().y / 2.f}, [this] {
        ImGui::TextUnformatted(dom_str_.c_str());
    });
}

void App::run_stylesheet_widget() const {
    auto const &size = window_.getSize();
    im::window("Stylesheet", {0, 70.f * scale_ + size.y / 2.f}, {size.x / 2.f, size.y / 2.f}, [this] {
        ImGui::TextUnformatted(stylesheet_str_.c_str());
    });
}

void App::run_layout_widget() const {
    auto const &size = window_.getSize();
    im::window("Layout", {size.x / 2.f, size.y / 2.f}, {size.x / 2.f, size.y / 2.f}, [this] {
        ImGui::TextUnformatted(layout_str_.c_str());
    });
}

void App::clear_render_surface() {
    if (render_debug_) {
        canvas_->clear(gfx::Color{});
        return;
    }

    auto const *layout = engine_.layout();
    if (!page_loaded_ || layout == nullptr) {
        canvas_->clear(gfx::Color{255, 255, 255});
        return;
    }

    // https://www.w3.org/TR/css-backgrounds-3/#special-backgrounds
    // If html or body has a background set, use that as the canvas background.
    // TODO(robinlinden): This should be done in //render, but requires new
    //                    //gfx APIs that I want to think a bit about.
    if (auto html_bg = layout->get_property<css::PropertyId::BackgroundColor>();
            html_bg != gfx::Color::from_css_name("transparent")) {
        canvas_->clear(html_bg);
        return;
    }

    auto body = dom::nodes_by_xpath(*layout, "/html/body");
    if (body.empty()) {
        canvas_->clear(gfx::Color{255, 255, 255});
        return;
    }

    if (auto body_bg = body[0]->get_property<css::PropertyId::BackgroundColor>();
            body_bg != gfx::Color::from_css_name("transparent")) {
        canvas_->clear(body_bg);
        return;
    }

    canvas_->clear(gfx::Color{255, 255, 255});
}

void App::render_layout() {
    auto const *layout = engine_.layout();
    if (layout == nullptr) {
        return;
    }

    if (render_debug_) {
        render::debug::render_layout_depth(*canvas_, *layout);
    } else {
        render::render_layout(*canvas_, *layout);
    }
}

void App::render_overlay() {
    ImGui::SFML::Render(window_);
}

void App::show_render_surface() {
    window_.display();
}

void App::switch_canvas() {
    reset_scroll();
    if (selected_canvas_ == Canvas::OpenGL) {
        selected_canvas_ = Canvas::Sfml;
        canvas_ = std::make_unique<gfx::SfmlCanvas>(window_);
    } else {
        selected_canvas_ = Canvas::OpenGL;
        canvas_ = std::make_unique<gfx::OpenGLCanvas>();
    }
    canvas_->set_scale(scale_);
    auto [width, height] = window_.getSize();
    canvas_->set_viewport_size(width, height);
}

} // namespace browser::gui
