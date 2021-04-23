#include "http/get.h"
#include "html/parse.h"

#include <fmt/format.h>
#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <spdlog/spdlog.h>

int main() {
    sf::RenderWindow window{sf::VideoMode(640, 480), "gui"};
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    char url_buf[255]{"example.com"};
    sf::Clock clock;
    http::Response response{};
    std::vector<dom::Node> dom{};
    std::string dom_str{};
    std::string err_str{};

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        ImGui::SFML::Update(window, clock.restart());

        ImGui::Begin("Navigation");
        if (ImGui::InputText(
                "Url", url_buf, sizeof(url_buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            response = http::get(url_buf);
            dom_str.clear();

            switch (response.err) {
                case http::Error::Ok: {
                    dom = html::parse(response.body);
                    for (const auto &node : dom) {
                        dom_str += dom::to_string(node);
                        dom_str += '\n';
                    }
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
        if (response.err != http::Error::Ok) {
            ImGui::Text("%s", err_str.c_str());
        }
        ImGui::End();

        ImGui::Begin("HTTP Response");
        if (ImGui::CollapsingHeader("Header")) { ImGui::Text("%s", response.header.c_str()); }
        if (ImGui::CollapsingHeader("Body")) { ImGui::Text("%s", response.body.c_str()); }
        ImGui::End();

        ImGui::Begin("DOM");
        ImGui::Text("%s", dom_str.c_str());
        ImGui::End();

        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}
