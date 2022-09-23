// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/gui/app.h"

#include "css/rule.h"
#include "dom/dom.h"
#include "gfx/opengl_canvas.h"
#include "gfx/painter.h"
#include "render/render.h"
#include "uri/uri.h"

#include <SFML/Window/Event.hpp>
#include <fmt/format.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>

#include <cmath>
#include <functional>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>

using namespace std::literals;

namespace browser::gui {
namespace {

auto constexpr kDefaultResolutionX = 640;
auto constexpr kDefaultResolutionY = 480;

// Magic number that felt right during testing.
auto constexpr kMouseWheelScrollFactor = 10;

std::optional<std::string_view> try_get_text_content(dom::Document const &doc, std::string_view path) {
    auto nodes = dom::nodes_by_path(doc.html(), path);
    if (nodes.empty() || nodes[0]->children.empty() || !std::holds_alternative<dom::Text>(nodes[0]->children[0])) {
        return std::nullopt;
    }
    return std::get<dom::Text>(nodes[0]->children[0]).text;
}

void ensure_has_scheme(std::string &url) {
    if (url.find("://") == std::string::npos) {
        spdlog::info("Url missing scheme, assuming https");
        url = fmt::format("https://{}", url);
    }
}

std::optional<std::string_view> try_get_uri(std::vector<dom::Node const *> const &);
std::string element_text(std::vector<dom::Node const *> const &dom_nodes) {
    if (dom_nodes.empty()) {
        return ""s;
    }

    // Special handling of <a> because I want to see what link I'm hovering.
    if (auto uri = try_get_uri(dom_nodes); uri.has_value()) {
        return "a: "s + std::string{*uri};
    }

    if (std::holds_alternative<dom::Text>(*dom_nodes[0])) {
        return std::get<dom::Text>(*dom_nodes[0]).text;
    }

    return std::get<dom::Element>(*dom_nodes[0]).name;
}

std::optional<std::string_view> try_get_uri(std::vector<dom::Node const *> const &nodes) {
    if (nodes.empty()) {
        return std::nullopt;
    }

    for (auto const *node : nodes) {
        auto const *element = std::get_if<dom::Element>(node);
        if (element && element->name == "a"sv && element->attributes.contains("href")) {
            return element->attributes.at("href");
        }
    }

    return std::nullopt;
}

std::string stylesheet_to_string(std::vector<css::Rule> const &stylesheet) {
    std::stringstream ss;
    for (auto const &rule : stylesheet) {
        ss << css::to_string(rule) << std::endl;
    }
    return std::move(ss).str();
}

// Returns a vector containing [child, child->parent, child->parent->parent, ...].
std::vector<dom::Node const *> gather_node_and_parents(style::StyledNode const &child) {
    std::vector<dom::Node const *> nodes;
    for (style::StyledNode const *node = &child; node != nullptr; node = node->parent) {
        nodes.push_back(&node->node);
    }
    return nodes;
}

} // namespace

App::App(std::string browser_title, std::string start_page_hint, bool load_start_page)
    : browser_title_{std::move(browser_title)}, window_{sf::VideoMode(kDefaultResolutionX, kDefaultResolutionY),
                                                        browser_title_},
      url_buf_{std::move(start_page_hint)} {
    window_.setFramerateLimit(60);
    window_.setMouseCursor(cursor_);
    ImGui::SFML::Init(window_);
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
    auto windowSize = window_.getSize();

    // Only resize the window if the user hasn't resized it.
    if (windowSize.x == kDefaultResolutionX && windowSize.y == kDefaultResolutionY) {
        window_.setSize({kDefaultResolutionX * scale_, kDefaultResolutionY * scale_});
        canvas_->set_viewport_size(window_.getSize().x, window_.getSize().y);
    }

    engine_.set_layout_width(windowSize.x / scale_);
}

int App::run() {
    while (window_.isOpen()) {
        sf::Event event;
        while (window_.pollEvent(event)) {
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
                    auto const dom_nodes = get_hovered_nodes(document_position);
                    nav_widget_extra_info_ =
                            fmt::format("{},{}: {}", document_position.x, document_position.y, element_text(dom_nodes));

                    // If imgui is dealing with the mouse, we do nothing and let imgui change the cursor.
                    if (ImGui::GetIO().WantCaptureMouse) {
                        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
                        break;
                    }

                    // Otherwise we tell imgui not to mess with the cursor, and change it according to what we're
                    // currently hovering over.
                    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
                    if (try_get_uri(dom_nodes).has_value()) {
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
                    auto const dom_nodes = get_hovered_nodes(std::move(document_position));
                    if (auto uri = try_get_uri(dom_nodes); uri.has_value()) {
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

        run_overlay();
        run_nav_widget();
        run_http_response_widget();
        run_dom_widget();
        run_stylesheet_widget();
        run_layout_widget();

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
    auto uri = uri::Uri::parse(url_buf_);
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
    if (auto page_title = try_get_text_content(engine_.dom(), "html.head.title"sv)) {
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
    layout_str_ = layout::to_string(engine_.layout());
}

std::vector<dom::Node const *> App::get_hovered_nodes(geom::Position p) const {
    if (!page_loaded_) {
        return {};
    }

    auto const *moused_over = layout::box_at_position(engine_.layout(), p);
    if (moused_over == nullptr || moused_over->node == nullptr) {
        return {};
    }

    return gather_node_and_parents(*moused_over->node);
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
    if (!page_loaded_) {
        return;
    }

    // Don't allow scrolling if the entire page fits on the screen.
    if (static_cast<int>(window_.getSize().y) > engine_.layout().dimensions.margin_box().height) {
        return;
    }

    // Don't allow overscroll in either direction.
    if (scroll_offset_y_ + pixels > 0) {
        pixels = -scroll_offset_y_;
    }

    int current_bottom_visible_y = static_cast<int>(window_.getSize().y) - scroll_offset_y_;
    int scrolled_bottom_visible_y = current_bottom_visible_y - pixels;
    if (scrolled_bottom_visible_y > engine_.layout().dimensions.margin_box().height) {
        pixels -= engine_.layout().dimensions.margin_box().height - scrolled_bottom_visible_y;
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
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(window_.getSize().x / 2.f, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Navigation");
    if (ImGui::InputText("Url", &url_buf_, ImGuiInputTextFlags_EnterReturnsTrue)) {
        ensure_has_scheme(url_buf_);
        navigate();
    }

    ImGui::TextUnformatted(nav_widget_extra_info_.c_str());
    ImGui::End();
}

void App::run_http_response_widget() const {
    ImGui::SetNextWindowPos(ImVec2(window_.getSize().x / 2.f, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(window_.getSize().x / 2.f, window_.getSize().y / 2.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("HTTP Response");
    ImGui::TextUnformatted(status_line_str_.c_str());
    if (ImGui::CollapsingHeader("Headers")) {
        ImGui::TextUnformatted(response_headers_str_.c_str());
    }
    if (ImGui::CollapsingHeader("Body")) {
        ImGui::TextUnformatted(engine_.response().body.c_str());
    }
    ImGui::End();
}

void App::run_dom_widget() const {
    ImGui::SetNextWindowPos(ImVec2(0, 70 * static_cast<float>(scale_)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(window_.getSize().x / 2.f, window_.getSize().y / 2.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("DOM");
    ImGui::TextUnformatted(dom_str_.c_str());
    ImGui::End();
}

void App::run_stylesheet_widget() const {
    ImGui::SetNextWindowPos(
            ImVec2(0, 70 * static_cast<float>(scale_) + window_.getSize().y / 2.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(window_.getSize().x / 2.f, window_.getSize().y / 2.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Stylesheet", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextUnformatted(stylesheet_str_.c_str());
    ImGui::End();
}

void App::run_layout_widget() const {
    ImGui::SetNextWindowPos(ImVec2(window_.getSize().x / 2.f, window_.getSize().y / 2.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(window_.getSize().x / 2.f, window_.getSize().y / 2.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Layout");
    ImGui::TextUnformatted(layout_str_.c_str());
    ImGui::End();
}

void App::clear_render_surface() {
    if (render_debug_) {
        window_.clear();
    } else {
        window_.clear(sf::Color(255, 255, 255));
    }
}

void App::render_layout() {
    gfx::Painter painter(*canvas_);
    if (render_debug_) {
        render::debug::render_layout_depth(painter, engine_.layout());
    } else {
        render::render_layout(painter, engine_.layout());
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
}

} // namespace browser::gui
