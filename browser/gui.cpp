#include "html/parse.h"
#include "http/get.h"
#include "layout/layout.h"
#include "style/style.h"

#include <fmt/format.h>
#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <spdlog/spdlog.h>

#include <optional>

int main() {
    sf::RenderWindow window{sf::VideoMode(640, 480), "gui"};
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    char url_buf[255]{"http://example.com"};
    sf::Clock clock;
    http::Response response{};
    dom::Document dom{};
    std::optional<style::StyledNode> styled{};
    std::optional<layout::LayoutBox> layout{};
    std::string dom_str{};
    std::string err_str{};
    std::string layout_str{};

    bool layout_needed{};

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
                    break;
                }
                default: break;
            }
        }

        ImGui::SFML::Update(window, clock.restart());

        ImGui::Begin("Navigation");
        if (ImGui::InputText(
                "Url", url_buf, sizeof(url_buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            auto uri = util::Uri::parse(url_buf);
            if (!uri) {
                continue;
            }

            response = http::get(*uri);
            dom_str.clear();

            switch (response.err) {
                case http::Error::Ok: {
                    dom = html::parse(response.body);
                    dom_str += dom::to_string(dom);

                    std::vector<css::Rule> stylesheet{
                        {{"head"}, {{"display", "none"}}},
                        {{"h1"}, {{"height", "50px"}}},
                        {{"p"}, {{"height", "25px"}}},
                        {{"div"}, {{"height", "100px"}}},
                        {{"div", "p"}, {{"width", "100px"}}},
                    };
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
            }
        }

        if (layout_needed && styled) {
            layout = layout::create_layout(*styled, window.getSize().x);
            layout_str = layout::to_string(*layout);
            layout_needed = false;
        }

        if (response.err != http::Error::Ok) {
            ImGui::TextUnformatted(err_str.c_str());
        }
        ImGui::End();

        ImGui::Begin("HTTP Response");
        if (ImGui::CollapsingHeader("Header")) { ImGui::TextUnformatted(response.header.c_str()); }
        if (ImGui::CollapsingHeader("Body")) { ImGui::TextUnformatted(response.body.c_str()); }
        ImGui::End();

        ImGui::Begin("DOM");
        ImGui::TextUnformatted(dom_str.c_str());
        ImGui::End();

        ImGui::Begin("Layout");
        ImGui::TextUnformatted(layout_str.c_str());
        ImGui::End();

        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}
