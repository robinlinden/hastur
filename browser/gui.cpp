// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/parse.h"
#include "html/parse.h"
#include "layout/layout.h"
#include "protocol/get.h"
#include "render/render.h"
#include "style/style.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <fmt/format.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/sinks/stdout_color_sinks.h>
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
#include <optional>

using namespace std::literals;

namespace {

auto const kBrowserTitle = "hastur";
auto const kDefaultUrl = "http://example.com";

std::optional<std::string_view> try_get_text_content(dom::Document const &doc, std::string_view path) {
    auto nodes = dom::nodes_by_path(doc.html, path);
    if (nodes.empty() || nodes[0]->children.empty() || !std::holds_alternative<dom::Text>(nodes[0]->children[0].data)) {
        return std::nullopt;
    }
    return std::get<dom::Text>(nodes[0]->children[0].data).text;
}

} // namespace

int main(int argc, char **argv) {
    spdlog::set_default_logger(spdlog::stderr_color_mt(kBrowserTitle));

    sf::RenderWindow window{sf::VideoMode(640, 480), kBrowserTitle};
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    std::string url_buf{argc > 1 ? argv[1] : kDefaultUrl};
    bool url_from_argv = argc > 1;
    sf::Clock clock;
    protocol::Response response{};
    dom::Document dom{};
    std::optional<style::StyledNode> styled{};
    std::optional<layout::LayoutBox> layout{};
    std::string status_line_str{};
    std::string response_headers_str{};
    std::string dom_str{};
    std::string err_str{};
    std::string layout_str{};
    std::string mouse_over_str{};

    // The scroll offset is the opposite of the current translation of the web page.
    // When we scroll "down", the web page is translated "up".
    float scroll_offset_y{};

    bool layout_needed{};

    render::render_setup(window.getSize().x, window.getSize().y);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            switch (event.type) {
                case sf::Event::Closed: {
                    window.close();
                    break;
                }
                case sf::Event::Resized: {
                    layout_needed = true;
                    render::render_setup(event.size.width, event.size.height);
                    break;
                }
                case sf::Event::KeyPressed: {
                    switch (event.key.code) {
                        case sf::Keyboard::Key::J: {
                            float translation = event.key.shift ? -20.f : -5.f;
                            glTranslatef(0, translation, 0);
                            scroll_offset_y -= translation;
                            break;
                        }
                        case sf::Keyboard::Key::K: {
                            float translation = event.key.shift ? 20.f : 5.f;
                            glTranslatef(0, translation, 0);
                            scroll_offset_y -= translation;
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
                case sf::Event::MouseMoved: {
                    if (!layout) {
                        break;
                    }

                    layout::Position p{static_cast<float>(event.mouseMove.x),
                            static_cast<float>(event.mouseMove.y) + scroll_offset_y};
                    mouse_over_str = [&] {
                        auto const *moused_over = layout::box_at_position(*layout, p);
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
                    }();
                    break;
                }
                default:
                    break;
            }
        }

        ImGui::SFML::Update(window, clock.restart());

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(window.getSize().x / 2.f, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("Navigation");
        if (ImGui::InputText("Url", &url_buf, ImGuiInputTextFlags_EnterReturnsTrue) || url_from_argv) {
            url_from_argv = false;
            auto uri = uri::Uri::parse(url_buf);
            if (!uri) {
                continue;
            }

            if (uri->path.empty()) {
                uri->path = "/";
            }

            auto is_redirect = [](int status_code) {
                return status_code == 301 || status_code == 302;
            };

            response = protocol::get(*uri);
            while (response.err == protocol::Error::Ok && is_redirect(response.status_line.status_code)) {
                spdlog::info("Following {} redirect from {} to {}",
                        response.status_line.status_code,
                        uri->uri,
                        response.headers.at("Location"));
                url_buf = response.headers.at("Location");
                uri = uri::Uri::parse(url_buf);
                if (uri->path.empty()) {
                    uri->path = "/";
                }
                response = protocol::get(*uri);
            }

            status_line_str = fmt::format("{} {} {}",
                    response.status_line.version,
                    response.status_line.status_code,
                    response.status_line.reason);
            response_headers_str = protocol::to_string(response.headers);
            dom_str.clear();

            switch (response.err) {
                case protocol::Error::Ok: {
                    dom = html::parse(response.body);
                    dom_str += dom::to_string(dom);

                    if (auto title = try_get_text_content(dom, "html.head.title"sv)) {
                        window.setTitle(fmt::format("{} - {}", *title, kBrowserTitle));
                    } else {
                        window.setTitle(kBrowserTitle);
                    }

                    std::vector<css::Rule> stylesheet{
                            {{"head"}, {{"display", "none"}}},
                    };

                    if (auto style = try_get_text_content(dom, "html.head.style"sv)) {
                        auto new_rules = css::parse(*style);
                        stylesheet.reserve(stylesheet.size() + new_rules.size());
                        stylesheet.insert(end(stylesheet),
                                std::make_move_iterator(begin(new_rules)),
                                std::make_move_iterator(end(new_rules)));
                    }

                    auto head_links = dom::nodes_by_path(dom.html, "html.head.link");
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
                        auto stylesheet_url = fmt::format("{}{}", url_buf, elem.attributes.at("href"));
                        spdlog::info("Downloading stylesheet from {}", stylesheet_url);
                        auto style_data = protocol::get(*uri::Uri::parse(stylesheet_url));

                        auto new_rules = css::parse(style_data.body);
                        stylesheet.reserve(stylesheet.size() + new_rules.size());
                        stylesheet.insert(end(stylesheet),
                                std::make_move_iterator(begin(new_rules)),
                                std::make_move_iterator(end(new_rules)));
                    }

                    spdlog::info("Styling dom w/ {} rules", stylesheet.size());
                    styled = style::style_tree(dom.html, stylesheet);
                    layout_needed = true;
                    break;
                }
                case protocol::Error::Unresolved: {
                    err_str = fmt::format("Unable to resolve endpoint for '{}'", url_buf);
                    spdlog::error(err_str);
                    break;
                }
                case protocol::Error::Unhandled: {
                    err_str = fmt::format("Unhandled protocol for '{}'", url_buf);
                    spdlog::error(err_str);
                    break;
                }
                case protocol::Error::InvalidResponse: {
                    err_str = fmt::format("Invalid response from '{}'", url_buf);
                    spdlog::error(err_str);
                    break;
                }
            }
        }

        if (layout_needed && styled) {
            scroll_offset_y = 0;
            mouse_over_str.clear();
            render::render_setup(window.getSize().x, window.getSize().y);
            layout = layout::create_layout(*styled, window.getSize().x);
            layout_str = layout::to_string(*layout);
            layout_needed = false;
        }

        if (response.err != protocol::Error::Ok) {
            ImGui::TextUnformatted(err_str.c_str());
        } else {
            ImGui::TextUnformatted(mouse_over_str.c_str());
        }
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(window.getSize().x / 2.f, 0), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(window.getSize().x / 2.f, window.getSize().y / 2.f), ImGuiCond_FirstUseEver);
        ImGui::Begin("HTTP Response");
        ImGui::TextUnformatted(status_line_str.c_str());
        if (ImGui::CollapsingHeader("Headers")) {
            ImGui::TextUnformatted(response_headers_str.c_str());
        }
        if (ImGui::CollapsingHeader("Body")) {
            ImGui::TextUnformatted(response.body.c_str());
        }
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(0, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(window.getSize().x / 2.f, window.getSize().y / 2.f), ImGuiCond_FirstUseEver);
        ImGui::Begin("DOM");
        ImGui::TextUnformatted(dom_str.c_str());
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(window.getSize().x / 2.f, window.getSize().y / 2.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(window.getSize().x / 2.f, window.getSize().y / 2.f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Layout");
        ImGui::TextUnformatted(layout_str.c_str());
        ImGui::End();

        window.clear();
        if (layout) {
            render::render_layout(*layout);
        }
        window.pushGLStates();
        ImGui::SFML::Render(window);
        window.popGLStates();
        window.display();
    }

    ImGui::SFML::Shutdown();
}
