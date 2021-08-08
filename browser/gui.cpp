#include "css/parse.h"
#include "html/parse.h"
#include "http/get.h"
#include "layout/layout.h"
#include "render/render.h"
#include "style/style.h"

#include <fmt/format.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui-SFML.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <spdlog/spdlog.h>

#include <iterator>
#include <optional>

using namespace std::literals;

namespace {

auto const kBrowserTitle = "hastur";
auto const kDefaultUrl = "http://example.com";

std::optional<std::string_view> try_get_text_content(dom::Document const &doc, std::string_view path) {
    auto nodes = dom::nodes_by_path(doc.html, path);
    if (nodes.empty()
            || nodes[0]->children.empty()
            || !std::holds_alternative<dom::Text>(nodes[0]->children[0].data)) {
        return std::nullopt;
    }
    return std::get<dom::Text>(nodes[0]->children[0].data).text;
}

} // namespace

int main(int argc, char **argv) {
    sf::RenderWindow window{sf::VideoMode(640, 480), kBrowserTitle};
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    std::string url_buf{argc > 1 ? argv[1] : kDefaultUrl};
    bool url_from_argv = argc > 1;
    sf::Clock clock;
    http::Response response{};
    dom::Document dom{};
    std::optional<style::StyledNode> styled{};
    std::optional<layout::LayoutBox> layout{};
    std::string status_line_str{};
    std::string response_headers_str{};
    std::string dom_str{};
    std::string err_str{};
    std::string layout_str{};

    bool layout_needed{};

    int scroll_offset{};

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
                            scroll_offset += event.key.shift ? 20 : 5;
                            break;
                        }
                        case sf::Keyboard::Key::K: {
                            scroll_offset -= event.key.shift ? 20 : 5;
                            break;
                        }
                        default: break;
                    }
                    scroll_offset = std::max(0, scroll_offset);
                    break;
                }
                default: break;
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

            response = http::get(*uri);
            status_line_str = fmt::format("{} {} {}",
                    response.status_line.version,
                    response.status_line.status_code,
                    response.status_line.reason);
            response_headers_str = http::to_string(response.headers);
            dom_str.clear();

            switch (response.err) {
                case http::Error::Ok: {
                    dom = html::parse(response.body);
                    dom_str += dom::to_string(dom);

                    if (auto title = try_get_text_content(dom, "html.head.title"sv); title) {
                        window.setTitle(fmt::format("{} - {}", *title, kBrowserTitle));
                    } else {
                        window.setTitle(kBrowserTitle);
                    }

                    std::vector<css::Rule> stylesheet{
                        {{"head"}, {{"display", "none"}}},
                    };

                    if (auto style = try_get_text_content(dom, "html.head.style"sv); style) {
                        auto new_rules = css::parse(*style);
                        stylesheet.reserve(stylesheet.size() + new_rules.size());
                        stylesheet.insert(
                                end(stylesheet),
                                std::make_move_iterator(begin(new_rules)),
                                std::make_move_iterator(end(new_rules)));
                    }

                    auto head_links = dom::nodes_by_path(dom.html, "html.head.link");
                    head_links.erase(std::remove_if(begin(head_links), end(head_links), [](auto const &n) {
                        auto elem = std::get<dom::Element>(n->data);
                        return elem.attributes.contains("rel") && elem.attributes.at("rel") != "stylesheet";
                    }), end(head_links));

                    spdlog::info("Loading {} stylesheets", head_links.size());
                    for (auto link : head_links) {
                        auto const &elem = std::get<dom::Element>(link->data);
                        auto stylesheet_url = fmt::format("{}{}", url_buf, elem.attributes.at("href"));
                        spdlog::info("Downloading stylesheet from {}", stylesheet_url);
                        auto style_data = http::get(*uri::Uri::parse(stylesheet_url));

                        auto new_rules = css::parse(style_data.body);
                        stylesheet.reserve(stylesheet.size() + new_rules.size());
                        stylesheet.insert(
                                end(stylesheet),
                                std::make_move_iterator(begin(new_rules)),
                                std::make_move_iterator(end(new_rules)));
                    }

                    spdlog::info("Styling dom w/ {} rules", stylesheet.size());
                    styled = style::style_tree(dom.html, stylesheet);
                    layout_needed = true;
                    break;
                }
                case http::Error::Unresolved: {
                    err_str = fmt::format("Unable to resolve endpoint for '{}'", url_buf);
                    spdlog::error(err_str);
                    break;
                }
                case http::Error::Unhandled: {
                    err_str = fmt::format("Unhandled protocol for '{}'", url_buf);
                    spdlog::error(err_str);
                    break;
                }
                case http::Error::InvalidResponse: {
                    err_str = fmt::format("Invalid response from '{}'", url_buf);
                    spdlog::error(err_str);
                    break;
                }
            }
        }

        if (layout_needed && styled) {
            scroll_offset = 0;
            layout = layout::create_layout(*styled, window.getSize().x);
            layout_str = layout::to_string(*layout);
            layout_needed = false;
        }

        if (response.err != http::Error::Ok) {
            ImGui::TextUnformatted(err_str.c_str());
        }
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(window.getSize().x / 2.f, 0), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(window.getSize().x / 2.f, window.getSize().y / 2.f), ImGuiCond_FirstUseEver);
        ImGui::Begin("HTTP Response");
        ImGui::TextUnformatted(status_line_str.c_str());
        if (ImGui::CollapsingHeader("Headers")) { ImGui::TextUnformatted(response_headers_str.c_str()); }
        if (ImGui::CollapsingHeader("Body")) { ImGui::TextUnformatted(response.body.c_str()); }
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
            render::render_layout(*layout, scroll_offset);
        }
        window.pushGLStates();
        ImGui::SFML::Render(window);
        window.popGLStates();
        window.display();
    }

    ImGui::SFML::Shutdown();
}
