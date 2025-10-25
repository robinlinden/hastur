// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/gif.h"
#include "img/jpeg_turbo.h"
#include "img/png.h"
#include "img/qoi.h"

#include "gfx/color.h"
#include "gfx/sfml_canvas.h"
#include "type/sfml.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

namespace {
using Image = std::variant<img::Gif, img::JpegTurbo, img::Png, img::Qoi>;

struct PixelDataGetter {
    template<typename T>
    std::vector<unsigned char> const *operator()(T const &) {
        return nullptr;
    }

    template<typename T>
    requires requires(T img) { img.bytes; }
    std::vector<unsigned char> const *operator()(T const &img) {
        return &img.bytes;
    }
};
} // namespace

int main(int argc, char **argv) {
    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: " << (argv[0] != nullptr ? argv[0] : "<bin>") << " [--metadata] <image_file>\n";
        return 1;
    }

    auto file_name = std::string{argv[argc - 1]};

    auto fs = std::ifstream{file_name, std::fstream::in | std::fstream::binary};
    if (!fs) {
        std::cerr << "Unable to open " << file_name << " for reading\n";
        return 1;
    }

    auto maybe_img = [&]() -> std::optional<Image> {
        if (auto png = img::Png::from(fs)) {
            return *png;
        }

        fs.clear();
        fs.seekg(0);

        if (auto gif = img::Gif::from(fs)) {
            return *gif;
        }

        fs.clear();
        fs.seekg(0);

        if (auto qoi = img::Qoi::from(fs)) {
            return *qoi;
        }

        fs.clear();
        fs.seekg(0);

        if (auto jpeg = img::JpegTurbo::from(fs)) {
            return *jpeg;
        }

        return std::nullopt;
    }();

    if (!maybe_img) {
        std::cerr << "Unable to parse " << file_name << " as an image\n";
        return 1;
    }

    auto img = *maybe_img;
    auto [width, height] = std::visit([](auto const &v) { return std::pair{v.width, v.height}; }, img);

    if (argc == 3 && argv[1] == "--metadata"sv) {
        std::cout << "Dimensions: " << width << 'x' << height << '\n';
        return 0;
    }

    auto const *maybe_pixel_data = std::visit(PixelDataGetter{}, img);
    if (maybe_pixel_data == nullptr) {
        std::cerr << "Only --metadata is supported for this file-type\n";
        return 1;
    }
    auto const &bytes = *maybe_pixel_data;

    if (bytes.size() != (static_cast<std::size_t>(width) * height * 4)) {
        std::cerr << "Unsupported pixel format, expected 32-bit rgba pixels\n";
        return 1;
    }

    auto const &desktop = sf::VideoMode::getDesktopMode();
    std::uint32_t window_width = std::clamp(width, 100u, desktop.size.x);
    std::uint32_t window_height = std::clamp(height, 100u, desktop.size.y);
    sf::RenderWindow window{sf::VideoMode{{window_width, window_height}}, "img"};
    window.setVerticalSyncEnabled(true);
    if (!window.setActive(true)) {
        std::cerr << "Failed to set window active\n";
        return 1;
    }

    type::SfmlType type;
    auto maybe_canvas = gfx::SfmlCanvas::create(window, type);
    if (!maybe_canvas) {
        std::cerr << "Failed to create canvas\n";
        return 1;
    }

    auto &canvas = *maybe_canvas;

    bool running = true;
    while (running) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                running = false;
            } else if (auto const *resized = event->getIf<sf::Event::Resized>()) {
                canvas.set_viewport_size(resized->size.x, resized->size.y);
            }
        }

        canvas.clear(gfx::Color{});
        canvas.draw_pixels({0, 0, static_cast<int>(width), static_cast<int>(height)}, bytes);
        window.display();
    }
}
