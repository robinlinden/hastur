// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/color.h"
#include "gfx/opengl_canvas.h"
#include "gfx/sfml_canvas.h"
#include "type/sfml.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <array>
#include <memory>
#include <string_view>

using namespace std::literals;

constexpr auto kHotPink = gfx::Color::from_rgb(0xff'69'b4);

int main(int argc, char **argv) {
    sf::RenderWindow window{sf::VideoMode{800, 600}, "gfx"};
    window.setVerticalSyncEnabled(true);
    window.setActive(true);

    type::SfmlType type;

    auto canvas = [&]() -> std::unique_ptr<gfx::ICanvas> {
        if (argc == 2 && argv[1] == "--sf"sv) {
            return std::make_unique<gfx::SfmlCanvas>(window, type);
        }
        return std::make_unique<gfx::OpenGLCanvas>();
    }();

    canvas->set_viewport_size(window.getSize().x, window.getSize().y);

    bool running = true;
    while (running) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    running = false;
                    break;
                case sf::Event::Resized:
                    canvas->set_viewport_size(event.size.width, event.size.height);
                    break;
                default:
                    break;
            }
        }

        auto size = window.getSize();
        auto x = static_cast<int>(size.x);
        auto y = static_cast<int>(size.y);

        canvas->clear(gfx::Color{0xFF, 0xFF, 0xFF});

        canvas->fill_rect({200, 200, 100, 100}, gfx::Color{0, 0, 0xAA});
        canvas->fill_rect({x / 4 + 50, y / 3 + 50, x / 2, y / 3}, gfx::Color{0xAA, 0, 0, 0x33});

        canvas->draw_rect({400, 100, 50, 50}, gfx::Color{80, 80, 80}, {}, {.top_right{50, 50}, .bottom_left{25, 25}});

        canvas->draw_text({100, 50}, "hello!"sv, {"arial"}, {16}, gfx::FontStyle::Normal, gfx::Color{});
        canvas->draw_text({100, 80}, "hello, but fancy!"sv, {"arial"}, {16}, gfx::FontStyle::Italic, gfx::Color{});
        canvas->draw_text({100, 110},
                "hello, but *even fancier*!"sv,
                {"arial"},
                {32},
                gfx::FontStyle::Italic | gfx::FontStyle::Bold,
                gfx::Color{});
        canvas->draw_text({120, 150},
                "hmmmm"sv,
                {"arial"},
                {24},
                gfx::FontStyle::Italic | gfx::FontStyle::Bold | gfx::FontStyle::Underlined,
                gfx::Color{});
        canvas->draw_text({150, 200},
                "oh no"sv,
                {"arial"},
                {24},
                gfx::FontStyle::Italic | gfx::FontStyle::Bold | gfx::FontStyle::Underlined
                        | gfx::FontStyle::Strikethrough,
                kHotPink);
        auto px = std::to_array<std::uint8_t>(
                {100, 100, 100, 0xff, 200, 200, 200, 0xff, 50, 50, 50, 0xff, 200, 0, 0, 0xff});
        canvas->draw_pixels({1, 1, 2, 2}, px);

        window.display();
    }
}
