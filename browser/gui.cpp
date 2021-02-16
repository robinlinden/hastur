#include <SFML/Window.hpp>

int main() {
    sf::Window window(sf::VideoMode(640, 480), "gui", sf::Style::Default);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
    }
}
