// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/png.h"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Event.hpp>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

using namespace std::literals;

int main(int argc, char **argv) {
    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: " << (argv[0] ? argv[0] : "<bin>") << " [--metadata] <image_file>\n";
        return 1;
    }

    auto file_name = std::string{argv[argc - 1]};

    auto fs = std::ifstream{file_name, std::fstream::in | std::fstream::binary};
    if (!fs) {
        std::cerr << "Unable to open " << file_name << " for reading\n";
        return 1;
    }

    auto img = img::Png::from(std::move(fs));
    if (!img) {
        std::cerr << "Unable to parse " << file_name << " as a png\n";
        return 1;
    }

    if (argc == 3 && argv[1] == "--metadata"sv) {
        std::cout << "Dimensions: " << img->width << 'x' << img->height << '\n';
        return 0;
    }

    if (img->bytes.size() != (static_cast<std::size_t>(img->width) * img->height * 4)) {
        std::cerr << "Unsupported pixel format, expected 32-bit rgba pixels\n";
        return 1;
    }

    auto const &desktop = sf::VideoMode::getDesktopMode();
    std::uint32_t window_width = std::clamp(img->width, 100u, desktop.width);
    std::uint32_t window_height = std::clamp(img->height, 100u, desktop.height);
    sf::RenderWindow window{sf::VideoMode{window_width, window_height}, "img"};
    window.setVerticalSyncEnabled(true);
    window.setActive(true);

    sf::Image sf_image{};
    sf_image.create(img->width, img->height, img->bytes.data());
    sf::Texture texture{};
    texture.loadFromImage(sf_image);
    sf::Sprite sprite{};
    sprite.setTexture(texture);

    bool running = true;
    while (running) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    running = false;
                    break;
                case sf::Event::Resized:
                    window.setView(sf::View{sf::FloatRect{
                            0, 0, static_cast<float>(event.size.width), static_cast<float>(event.size.height)}});
                    break;
                default:
                    break;
            }
        }

        window.clear();
        window.draw(sprite);
        window.display();
    }
}
