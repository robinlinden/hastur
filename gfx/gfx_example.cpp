// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/color.h"
#include "gfx/opengl_painter.h"
#include "gfx/sfml_painter.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <memory>
#include <string_view>

using namespace std::literals;

int main(int argc, char **argv) {
    sf::RenderWindow window{sf::VideoMode{800, 600}, "gfx"};
    window.setVerticalSyncEnabled(true);
    window.setActive(true);

    auto painter = [&]() -> std::unique_ptr<gfx::IPainter> {
        if (argc == 2 && argv[1] == "--sf"sv) {
            return std::make_unique<gfx::SfmlPainter>(window);
        }
        return std::make_unique<gfx::OpenGLPainter>();
    }();

    painter->set_viewport_size(window.getSize().x, window.getSize().y);

    bool running = true;
    while (running) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    running = false;
                    break;
                case sf::Event::Resized:
                    painter->set_viewport_size(event.size.width, event.size.height);
                    break;
                default:
                    break;
            }
        }

        auto size = window.getSize();
        auto x = static_cast<int>(size.x);
        auto y = static_cast<int>(size.y);

        painter->fill_rect({0, 0, x, y}, gfx::Color{0xFF, 0xFF, 0xFF});

        painter->fill_rect({200, 200, 100, 100}, gfx::Color{0, 0, 0xAA});
        painter->fill_rect({x / 4 + 50, y / 3 + 50, x / 2, y / 3}, gfx::Color{0xAA, 0, 0, 0x33});

        window.display();
    }
}
