// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/color.h"
#include "gfx/icanvas.h"
#include "gfx/opengl_canvas.h"
#include "gfx/sfml_canvas.h"
#include "type/sfml.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>

using namespace std::literals;

constexpr auto kHotPink = gfx::Color::from_rgb(0xff'69'b4);

int main(int argc, char **argv) {
    sf::RenderWindow window{sf::VideoMode{{800, 600}}, "gfx"};
    window.setVerticalSyncEnabled(true);
    if (!window.setActive(true)) {
        std::cerr << "Failed to set window active\n";
        return 1;
    }

    type::SfmlType type;

    auto maybe_canvas = [&]() -> std::optional<std::unique_ptr<gfx::ICanvas>> {
        if (argc == 2 && argv[1] == "--sf"sv) {
            auto c = gfx::SfmlCanvas::create(window, type);
            if (!c) {
                std::cerr << "Failed to create SFML canvas\n";
                return std::nullopt;
            }

            return std::make_unique<gfx::SfmlCanvas>(std::move(*c));
        }
        return std::make_unique<gfx::OpenGLCanvas>();
    }();

    if (!maybe_canvas) {
        std::cerr << "Failed to create canvas\n";
        return 1;
    }

    auto &canvas = *maybe_canvas;

    canvas->set_viewport_size(window.getSize().x, window.getSize().y);

    bool running = true;
    while (running) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                running = false;
            } else if (auto const *resized = event->getIf<sf::Event::Resized>()) {
                canvas->set_viewport_size(resized->size.x, resized->size.y);
            }
        }

        auto size = window.getSize();
        auto x = static_cast<int>(size.x);
        auto y = static_cast<int>(size.y);

        canvas->clear(gfx::Color{0xFF, 0xFF, 0xFF});

        canvas->draw_rect({200, 200, 100, 100}, gfx::Color{0, 0, 0xAA}, {}, {});
        canvas->draw_rect({x / 4 + 50, y / 3 + 50, x / 2, y / 3}, gfx::Color{0xAA, 0, 0, 0x33}, {}, {});

        canvas->draw_rect({400, 100, 50, 50}, gfx::Color{80, 80, 80}, {}, {.top_right{50, 50}, .bottom_left{25, 25}});

        canvas->draw_text({100, 50}, "hello!"sv, {"arial"}, {16}, {}, gfx::Color{});
        canvas->draw_text({100, 80}, "hello, but fancy!"sv, {"arial"}, {16}, {.italic = true}, gfx::Color{});
        canvas->draw_text({100, 110},
                "hello, but *even fancier*!"sv,
                {"arial"},
                {32},
                {.bold = true, .italic = true},
                gfx::Color{});
        canvas->draw_text({120, 150},
                "hmmmm"sv,
                {"arial"},
                {24},
                {.bold = true, .italic = true, .underlined = true},
                gfx::Color{});
        canvas->draw_text({150, 200},
                "oh no"sv,
                {"arial"},
                {24},
                {.bold = true, .italic = true, .strikethrough = true, .underlined = true},
                kHotPink);
        auto px = std::to_array<std::uint8_t>(
                {100, 100, 100, 0xff, 200, 200, 200, 0xff, 50, 50, 50, 0xff, 200, 0, 0, 0xff});
        canvas->draw_pixels({1, 1, 2, 2}, px);

        window.display();
    }
}
