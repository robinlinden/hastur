// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/gui/app.h"

#include "css/default.h"
#include "css/parse.h"
#include "html/parse.h"
#include "render/render.h"

#include <SFML/Window/Event.hpp>
#include <fmt/format.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>

// MSVC gl.h doesn't include everything it uses.
#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _MSC_VER

#include <GL/gl.h>

#include <iterator>
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
                    layout();
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
                        default:
                            break;
                    }
                    break;
                }
                case sf::Event::MouseMoved: {
                    auto window_position = layout::Position{event.mouseMove.x, event.mouseMove.y};
                    auto document_position = to_document_position(std::move(window_position));
                    mouse_over_str_ = get_hovered_element_text(std::move(document_position));
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
        render_layout();
        render_overlay();
        show_render_surface();
    }

    return 0;
}

void App::navigate() {
    auto uri = uri::Uri::parse(url_buf_);
    if (!uri) {
        return;
    }

    if (uri->path.empty()) {
        uri->path = "/";
    }

    auto is_redirect = [](int status_code) {
        return status_code == 301 || status_code == 302;
    };

    response_ = protocol::get(*uri);
    while (response_.err == protocol::Error::Ok && is_redirect(response_.status_line.status_code)) {
        spdlog::info("Following {} redirect from {} to {}",
                response_.status_line.status_code,
                uri->uri,
                response_.headers.at("Location"));
        url_buf_ = response_.headers.at("Location");
        uri = uri::Uri::parse(url_buf_);
        if (uri->path.empty()) {
            uri->path = "/";
        }
        response_ = protocol::get(*uri);
    }

    status_line_str_ = fmt::format(
            "{} {} {}", response_.status_line.version, response_.status_line.status_code, response_.status_line.reason);
    response_headers_str_ = protocol::to_string(response_.headers);
    dom_str_.clear();

    switch (response_.err) {
        case protocol::Error::Ok: {
            dom_ = html::parse(response_.body);
            dom_str_ += dom::to_string(dom_);

            if (auto page_title = try_get_text_content(dom_, "html.head.title"sv)) {
                window_.setTitle(fmt::format("{} - {}", *page_title, browser_title_));
            } else {
                window_.setTitle(browser_title_);
            }

            auto stylesheet{css::default_style()};

            if (auto style = try_get_text_content(dom_, "html.head.style"sv)) {
                auto new_rules = css::parse(*style);
                stylesheet.reserve(stylesheet.size() + new_rules.size());
                stylesheet.insert(end(stylesheet),
                        std::make_move_iterator(begin(new_rules)),
                        std::make_move_iterator(end(new_rules)));
            }

            auto head_links = dom::nodes_by_path(dom_.html, "html.head.link");
            head_links.erase(std::remove_if(begin(head_links),
                                     end(head_links),
                                     [](auto const &n) {
                                         auto elem = std::get<dom::Element>(n->data);
                                         return elem.attributes.contains("rel")
                                                 && elem.attributes.at("rel") != "stylesheet";
                                     }),
                    end(head_links));

            spdlog::info("Loading {} stylesheets", head_links.size());
            for (auto link : head_links) {
                auto const &elem = std::get<dom::Element>(link->data);
                auto stylesheet_url = fmt::format("{}{}", url_buf_, elem.attributes.at("href"));
                spdlog::info("Downloading stylesheet from {}", stylesheet_url);
                auto style_data = protocol::get(*uri::Uri::parse(stylesheet_url));

                auto new_rules = css::parse(style_data.body);
                stylesheet.reserve(stylesheet.size() + new_rules.size());
                stylesheet.insert(end(stylesheet),
                        std::make_move_iterator(begin(new_rules)),
                        std::make_move_iterator(end(new_rules)));
            }

            spdlog::info("Styling dom w/ {} rules", stylesheet.size());
            styled_ = style::style_tree(dom_.html, stylesheet);
            layout();
            break;
        }
        case protocol::Error::Unresolved: {
            err_str_ = fmt::format("Unable to resolve endpoint for '{}'", url_buf_);
            spdlog::error(err_str_);
            break;
        }
        case protocol::Error::Unhandled: {
            err_str_ = fmt::format("Unhandled protocol for '{}'", url_buf_);
            spdlog::error(err_str_);
            break;
        }
        case protocol::Error::InvalidResponse: {
            err_str_ = fmt::format("Invalid response from '{}'", url_buf_);
            spdlog::error(err_str_);
            break;
        }
    }
}

void App::layout() {
    if (!styled_) {
        return;
    }
    reset_scroll();
    mouse_over_str_.clear();
    layout_ = layout::create_layout(*styled_, window_.getSize().x);
    layout_str_ = layout::to_string(*layout_);
}

std::string App::get_hovered_element_text(layout::Position p) const {
    if (!layout_) {
        return ""s;
    }

    auto const *moused_over = layout::box_at_position(*layout_, p);
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

layout::Position App::to_document_position(layout::Position window_position) const {
    return {window_position.x, window_position.y + scroll_offset_y_};
}

void App::reset_scroll() {
    scroll_offset_y_ = 0;
    render::render_setup(window_.getSize().x, window_.getSize().y);
}

void App::scroll(int pixels) {
    glTranslatef(0, static_cast<float>(pixels), 0);
    scroll_offset_y_ -= pixels;
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

    if (response_.err != protocol::Error::Ok) {
        ImGui::TextUnformatted(err_str_.c_str());
    } else {
        ImGui::TextUnformatted(mouse_over_str_.c_str());
    }
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
        ImGui::TextUnformatted(response_.body.c_str());
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
    if (layout_) {
        render::render_layout(*layout_);
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
