// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/gui/app.h"

#include "dom/dom.h"
#include "render/render.h"
#include "uri/uri.h"

#include <SFML/Window/Event.hpp>
#include <fmt/format.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>

#include <functional>
#include <string_view>
#include <utility>

using namespace std::literals;

namespace browser::gui {
namespace {

std::optional<std::string_view> try_get_text_content(dom::Document const &doc, std::string_view path) {
    auto nodes = dom::nodes_by_path(doc.html, path);
    if (nodes.empty() || nodes[0]->children.empty() || !std::holds_alternative<dom::Text>(nodes[0]->children[0].data)) {
        return std::nullopt;
    }
    return std::get<dom::Text>(nodes[0]->children[0].data).text;
}

} // namespace

App::App(std::string browser_title, std::string start_page_hint, bool load_start_page)
    : browser_title_{std::move(browser_title)}, window_{sf::VideoMode(640, 480), browser_title_},
      url_buf_{std::move(start_page_hint)} {
    window_.setFramerateLimit(60);
    ImGui::SFML::Init(window_);
    render::render_setup(window_.getSize().x, window_.getSize().y);

    engine_.set_layout_width(window_.getSize().x);
    engine_.set_on_navigation_failure(std::bind(&App::on_navigation_failure, this, std::placeholders::_1));
    engine_.set_on_page_loaded(std::bind(&App::on_page_loaded, this));
    engine_.set_on_layout_updated(std::bind(&App::on_layout_updated, this));

    if (load_start_page) {
        navigate();
    }
}

App::~App() {
    ImGui::SFML::Shutdown();
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
                    render::render_setup(event.size.width, event.size.height);
                    engine_.set_layout_width(event.size.width);
                    break;
                }
                case sf::Event::KeyPressed: {
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
                    nav_widget_extra_info_ = get_hovered_element_text(std::move(document_position));
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
    if (!uri) {
        return;
    }

    if (uri->path.empty()) {
        uri->path = "/";
    }

    engine_.navigate(std::move(*uri));

    // Make sure the displayed url is still correct if we followed any redirects.
    url_buf_ = engine_.uri().uri;
}

void App::on_navigation_failure(protocol::Error err) {
    update_status_line();
    response_headers_str_ = protocol::to_string(engine_.response().headers);
    dom_str_.clear();
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
    response_headers_str_ = protocol::to_string(engine_.response().headers);
    dom_str_ = dom::to_string(engine_.dom());
    on_layout_updated();
}

void App::on_layout_updated() {
    reset_scroll();
    nav_widget_extra_info_.clear();
    layout_str_ = layout::to_string(engine_.layout());
}

std::string App::get_hovered_element_text(geom::Position p) const {
    if (!page_loaded_) {
        return ""s;
    }

    auto const *moused_over = layout::box_at_position(engine_.layout(), p);
    if (!moused_over) {
        return ""s;
    }

    auto const &dom_node = moused_over->node->node.get();
    if (std::holds_alternative<dom::Text>(dom_node.data)) {
        return std::get<dom::Text>(dom_node.data).text;
    }

    // Special handling of <a> because I want to see what link I'm hovering.
    auto const &element = std::get<dom::Element>(dom_node.data);
    if (element.name == "a"s && element.attributes.contains("href")) {
        return element.name + ": " + element.attributes.at("href");
    }

    return element.name;
}

geom::Position App::to_document_position(geom::Position window_position) const {
    return {window_position.x, window_position.y - scroll_offset_y_};
}

void App::reset_scroll() {
    painter_.add_translation(0, -scroll_offset_y_);
    scroll_offset_y_ = 0;
    render::render_setup(window_.getSize().x, window_.getSize().y);
}

void App::scroll(int pixels) {
    painter_.add_translation(0, pixels);
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
    ImGui::SetNextWindowPos(ImVec2(0, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(window_.getSize().x / 2.f, window_.getSize().y / 2.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("DOM");
    ImGui::TextUnformatted(dom_str_.c_str());
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
    window_.clear();
}

void App::render_layout() {
    if (render_debug_) {
        render::debug::render_layout_depth(painter_, engine_.layout());
    } else {
        render::render_layout(painter_, engine_.layout());
    }
}

void App::render_overlay() {
    window_.pushGLStates();
    ImGui::SFML::Render(window_);
    window_.popGLStates();
}

void App::show_render_surface() {
    window_.display();
}

} // namespace browser::gui
