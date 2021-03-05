#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

int main() {
    sf::RenderWindow window{sf::VideoMode(640, 480), "gui"};
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    char url_buf[255];
    sf::Clock clock;

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
        ImGui::InputText("Url", url_buf, sizeof(url_buf));
        ImGui::Button("Go");
        ImGui::End();

        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}
