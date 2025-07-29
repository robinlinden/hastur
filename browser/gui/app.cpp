// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/gui/app.h"

#include "browser/gui/about_handler.h"

#include "css/style_sheet.h"
#include "dom/dom.h"
#include "dom/xpath.h"
#include "engine/engine.h"
#include "geom/geom.h"
#include "gfx/color.h"
#include "gfx/opengl_canvas.h"
#include "gfx/sfml_canvas.h"
#include "img/jpeg_turbo.h"
#include "img/png.h"
#include "layout/layout.h"
#include "layout/layout_box.h"
#include "os/system_info.h"
#include "protocol/handler_factory.h"
#include "protocol/in_memory_cache.h"
#include "protocol/iprotocol_handler.h"
#include "protocol/response.h"
#include "render/render.h"
#include "type/sfml.h"
#include "type/type.h"
#include "uri/uri.h"
#include "url/percent_encode.h"
#include "util/string.h"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Window/Cursor.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

namespace browser::gui {
namespace {

auto constexpr kDefaultResolutionX = 1024;
auto constexpr kDefaultResolutionY = 768;

// Limit is pretty low right now as we currently spawn one thread per load
// instead of doing something sane. Will be made configurable in the future.
auto constexpr kMaxConcurrentImageLoads = 5;

// Magic number that felt right during testing.
auto constexpr kMouseWheelScrollFactor = 10;

std::future<ResourceResult> load_image(engine::Engine &e, uri::Uri uri, std::string id) {
    return std::async(std::launch::async, [&e, uri = std::move(uri), resource_id = std::move(id)]() mutable {
        spdlog::info("Loading image from '{}'", uri.uri);
        auto start_time = std::chrono::steady_clock::now();
        auto res = e.load(uri);
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        spdlog::info("Loaded image from '{}' in {}ms", uri.uri, duration.count());
        return ResourceResult{std::move(resource_id), std::move(res)};
    });
}

std::optional<std::string_view> try_get_text_content(dom::Document const &doc, std::string_view xpath) {
    auto nodes = dom::nodes_by_xpath(doc.html(), xpath);
    if (nodes.empty() || nodes[0]->children.empty()) {
        return std::nullopt;
    }

    if (auto const *text = std::get_if<dom::Text>(&nodes[0]->children[0])) {
        return text->text;
    }

    return std::nullopt;
}

void ensure_has_scheme(std::string &url) {
    // TODO(robinlinden): Handle opaque urls in a nicer way.
    if (!url.contains("://") && !url.starts_with("about:")) {
        spdlog::info("Url missing scheme, assuming https");
        url = std::format("https://{}", url);
    }
}

std::optional<std::string_view> try_get_uri(layout::LayoutBox const *from) {
    if (from == nullptr) {
        return std::nullopt;
    }

    for (auto const *node = from->node; node != nullptr; node = node->parent) {
        auto const *element = std::get_if<dom::Element>(&node->node);
        if ((element != nullptr) && element->name == "a"sv && element->attributes.contains("href")) {
            return element->attributes.at("href");
        }
    }

    return std::nullopt;
}

std::string element_text(layout::LayoutBox const *element) {
    if (element == nullptr || element->node == nullptr) {
        return ""s;
    }

    // Special handling of <a> because I want to see what link I'm hovering.
    if (auto uri = try_get_uri(element); uri.has_value()) {
        return "a: "s + std::string{*uri};
    }

    if (auto text = element->text()) {
        return std::string{*text};
    }

    return std::get<dom::Element>(element->node->node).name;
}

template<std::size_t WidthT, std::size_t HeightT>
consteval auto as_pixel_data(std::string_view data) {
    std::array<std::uint8_t, WidthT * HeightT * 4> pixels{};
    std::size_t px_idx{};

    constexpr auto kForegroundColor = gfx::Color{};
    constexpr auto kBackgroundColor = gfx::Color{255, 255, 255};

    for (char current : data) {
        if (current == '\n') {
            continue;
        }

        auto color = current == ' ' ? kBackgroundColor : kForegroundColor;
        pixels[px_idx * 4 + 0] = color.r;
        pixels[px_idx * 4 + 1] = color.g;
        pixels[px_idx * 4 + 2] = color.b;
        pixels[px_idx * 4 + 3] = color.a;
        px_idx += 1;
    }

    return pixels;
}

constexpr auto kBrowserIcon = as_pixel_data<16, 16>(R"(
XXXXXXXXXXXXXXXX
X              X
X  XX      XX  X
X  XX      XX  X
X  XX      XX  X
X  XX      XX  X
X  XX      XX  X
X  XXXXXXXXXX  X
X  XXXXXXXXXX  X
X  XX      XX  X
X  XX      XX  X
X  XX      XX  X
X  XX      XX  X
X  XX      XX  X
X              X
XXXXXXXXXXXXXXXX)");

std::unique_ptr<type::IType> create_font_system() {
    auto type = std::make_unique<type::SfmlType>();
    auto set_up_font = [&type](std::string name, std::span<std::string_view const> file_name_options) {
        for (auto const &file_name : file_name_options) {
            if (auto font = type->font(file_name)) {
                spdlog::info("Using '{}' as '{}'", file_name, name);
                type->set_font(name, std::static_pointer_cast<type::SfmlFont const>(*std::move(font)));
                return;
            }
        }

        spdlog::warn("Unable to find a font for '{}', looked for [{}], good luck",
                name,
                util::join(file_name_options, ", "));
    };

    static constexpr auto kMonospaceFontFileNames = std::to_array<std::string_view>({
#ifdef _WIN32
            "consola.ttf",
#else
            "LiberationMono-Regular.ttf",
            "DejaVuSansMono.ttf",
#endif
    });

    static constexpr auto kSansSerifFontFileNames = std::to_array<std::string_view>({
#ifdef _WIN32
            "arial.ttf",
#else
            "LiberationSans-Regular.ttf",
            "DejaVuSans.ttf",
#endif
    });

    static constexpr auto kSerifFontFileNames = std::to_array<std::string_view>({
#ifdef _WIN32
            "times.ttf",
#else
            "LiberationSerif-Regular.ttf",
            "DejaVuSerif.ttf",
#endif
    });

    set_up_font("monospace", kMonospaceFontFileNames);
    set_up_font("sans-serif", kSansSerifFontFileNames);
    set_up_font("serif", kSerifFontFileNames);

    return type;
}

std::vector<std::string_view> collect_image_urls(
        layout::LayoutBox const &root, std::span<std::string_view const> file_endings) {
    std::vector<std::string_view> image_urls;

    std::vector<layout::LayoutBox const *> to_check{&root};
    while (!to_check.empty()) {
        auto const *current = to_check.back();
        to_check.pop_back();

        for (auto const &child : current->children) {
            to_check.push_back(&child);
        }

        if (current->is_anonymous_block()) {
            continue;
        }

        if (auto const *element = std::get_if<dom::Element>(&current->node->node);
                element != nullptr && element->name == "img"sv) {
            if (auto it = element->attributes.find("src"); it != end(element->attributes)) {
                std::string_view src = it->second;
                if (std::ranges::any_of(
                            file_endings, [src](std::string_view ending) { return src.ends_with(ending); })) {
                    image_urls.push_back(src);
                }
            }
        }
    }

    return image_urls;
}

std::unique_ptr<protocol::IProtocolHandler> create_protocol_handler() {
    auto handlers = protocol::HandlerFactory::create(
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:102.0) Gecko/20100101 Firefox/102.0");
    auto about_handler = std::make_unique<browser::gui::AboutHandler>(Handlers{
            {
                    "blank",
                    []() {
                        return R"(<!DOCTYPE html>
<html>
<head>
    <style>
        @media (prefers-color-scheme: dark) {
            html { background-color: black; }
        }
    </style>
</head>
<body></body>
</html>
)";
                    },
            },
    });
    handlers->add("about", std::move(about_handler));
    return std::make_unique<protocol::InMemoryCache>(std::move(handlers));
};

} // namespace

// Latest Firefox ESR user agent (on Windows). This matches what the Tor browser does.
App::App(std::string browser_title, std::string start_page_hint, bool load_start_page)
    : engine_{create_protocol_handler(),
              create_font_system(),
              [this](std::string_view url) -> std::optional<layout::Size> {
                  auto it = images_.find(url);
                  if (it == end(images_)) {
                      return std::nullopt;
                  }

                  return layout::Size{static_cast<int>(it->second.width), static_cast<int>(it->second.height)};
              }},
      browser_title_{std::move(browser_title)},
      window_{sf::VideoMode({kDefaultResolutionX, kDefaultResolutionY}), browser_title_},
      url_buf_{std::move(start_page_hint)},
      canvas_{std::make_unique<gfx::SfmlCanvas>(window_, static_cast<type::SfmlType &>(engine_.font_system()))} {
    window_.setIcon({16, 16}, kBrowserIcon.data());
    if (!ImGui::SFML::Init(window_)) {
        spdlog::critical("imgui-sfml initialization failed");
        std::abort();
    }

    // This is okay as long as we don't call e.g. setenv(), unsetenv(), or putenv().
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    if (std::getenv("HST_DISABLE_DISK_IO") != nullptr) {
        // TODO(robinlinden): Support for things like HST_DISABLE_DISK_IO=0 to
        // re-enable IO.
        ImGui::GetIO().IniFilename = nullptr;
    }

    canvas_->set_viewport_size(window_.getSize().x, window_.getSize().y);

    if (load_start_page) {
        ensure_has_scheme(url_buf_);
        navigate();
    }
}

App::~App() {
    ImGui::SFML::Shutdown();
}

void App::set_scale(unsigned scale) {
    scale_ = scale;
    ImGui::GetIO().FontGlobalScale = static_cast<float>(scale_);
    canvas_->set_scale(scale_);
    auto window_size = window_.getSize();

    // Only resize the window if the user hasn't resized it.
    if (window_size.x == kDefaultResolutionX && window_size.y == kDefaultResolutionY) {
        window_.setSize({kDefaultResolutionX * scale_, kDefaultResolutionY * scale_});
        canvas_->set_viewport_size(window_.getSize().x, window_.getSize().y);
    }
}

void App::step() {
    while (auto event = window_.pollEvent()) {
        // ImGui needs a few iterations to do what it wants to do. This was
        // pretty much picked at random after I still occasionally got
        // unexpected results when giving it 2 iterations.
        process_iterations_ = 5;
        handle_event(*event);
    }

    bool should_relayout{};
    for (auto it = begin(ongoing_loads_); it != end(ongoing_loads_);) {
        auto &load = *it;
        if (load.wait_for(std::chrono::seconds{0}) != std::future_status::ready) {
            ++it;
            continue;
        }

        auto result = load.get();
        it = ongoing_loads_.erase(it);
        if (!result.result.response.has_value()) {
            auto const &err = result.result.response.error();
            spdlog::warn("Error {} downloading '{}': {}",
                    static_cast<int>(err.err),
                    result.result.uri_after_redirects.uri,
                    to_string(err.err));
            continue;
        }

        if (result.resource_id.ends_with(".png")) {
            auto ss = std::istringstream{std::move(result.result.response->body)};
            auto png = img::Png::from(ss);
            if (!png.has_value()) {
                spdlog::warn("Error parsing png from '{}'", result.result.uri_after_redirects.uri);
                continue;
            }

            auto &image = images_[std::move(result.resource_id)];
            image.width = png->width;
            image.height = png->height;
            image.rgba_bytes = std::move(png->bytes);
            spdlog::info(
                    "Parsed image (w={},h={}) '{}'", image.width, image.height, result.result.uri_after_redirects.uri);
            should_relayout = true;
        } else {
            assert(result.resource_id.ends_with(".jpg") || result.resource_id.ends_with(".jpeg"));
            auto const &body = result.result.response->body;
            auto jpeg = img::JpegTurbo::from({reinterpret_cast<std::byte const *>(body.data()), body.size()});
            if (!jpeg.has_value()) {
                spdlog::warn("Error parsing jpeg from '{}'", result.result.uri_after_redirects.uri);
                continue;
            }

            auto &image = images_[std::move(result.resource_id)];
            image.width = jpeg->width;
            image.height = jpeg->height;
            image.rgba_bytes = std::move(jpeg->bytes);
            spdlog::info(
                    "Parsed image (w={},h={}) '{}'", image.width, image.height, result.result.uri_after_redirects.uri);
            should_relayout = true;
        }
    }

    while (ongoing_loads_.size() < kMaxConcurrentImageLoads && !pending_loads_.empty()) {
        auto [uri, id] = std::move(pending_loads_.back());
        pending_loads_.pop_back();
        ongoing_loads_.push_back(load_image(engine_, std::move(uri), std::move(id)));
    }

    if (should_relayout) {
        assert(maybe_page_);
        engine_.relayout(page(), make_options());
        on_layout_updated();
        process_iterations_ = 5;
    }

    if (process_iterations_ == 0) {
        // The sleep duration was picked at random.
        std::this_thread::sleep_for(std::chrono::milliseconds{5});
        return;
    }
    process_iterations_ -= 1;

    run_overlay();
    run_nav_widget();
    if (display_debug_gui_) {
        run_debug_widget();
    }

    if (!maybe_page_ || (**maybe_page_).layout == std::nullopt) {
        canvas_->clear(gfx::Color{255, 255, 255});
    } else {
        render_layout();
    }

    render_overlay();
    show_render_surface();
}

void App::handle_event(sf::Event const &event) {
    ImGui::SFML::ProcessEvent(window_, event);

    if (event.is<sf::Event::Closed>()) {
        window_.close();
    } else if (auto const *resized = event.getIf<sf::Event::Resized>()) {
        canvas_->set_viewport_size(resized->size.x, resized->size.y);
        if (maybe_page_) {
            engine_.relayout(**maybe_page_, make_options());
            on_layout_updated();
        }
    } else if (auto const *key_pressed = event.getIf<sf::Event::KeyPressed>()) {
        if (ImGui::GetIO().WantCaptureKeyboard) {
            return;
        }

        switch (key_pressed->code) {
            case sf::Keyboard::Key::D: {
                if (!key_pressed->control) {
                    break;
                }
                scroll(-static_cast<int>(window_.getSize().y) / 2);
                break;
            }
            case sf::Keyboard::Key::J: {
                scroll(key_pressed->shift ? -20 : -5);
                break;
            }
            case sf::Keyboard::Key::K: {
                scroll(key_pressed->shift ? 20 : 5);
                break;
            }
            case sf::Keyboard::Key::L: {
                if (!key_pressed->control) {
                    break;
                }
                focus_url_input();
                break;
            }
            case sf::Keyboard::Key::U: {
                if (!key_pressed->control) {
                    break;
                }
                scroll(static_cast<int>(window_.getSize().y) / 2);
                break;
            }
            case sf::Keyboard::Key::F4: {
                display_debug_gui_ = !display_debug_gui_;
                spdlog::info("Display debug gui: {}", display_debug_gui_);
                break;
            }
            case sf::Keyboard::Key::Left: {
                if (!key_pressed->alt) {
                    break;
                }
                navigate_back();
                break;
            }
            case sf::Keyboard::Key::Right: {
                if (!key_pressed->alt) {
                    break;
                }
                navigate_forward();
                break;
            }
            case sf::Keyboard::Key::Backspace: {
                navigate_back();
                break;
            }
            case sf::Keyboard::Key::R: {
                if (!key_pressed->control) {
                    break;
                }
                reload();
                break;
            }
            default:
                break;
        }
    } else if (auto const *mouse_moved = event.getIf<sf::Event::MouseMoved>()) {
        if (!maybe_page_) {
            return;
        }

        auto window_position = geom::Position{mouse_moved->position.x, mouse_moved->position.y};
        auto document_position = to_document_position(std::move(window_position));
        auto const *hovered = get_hovered_node(document_position);
        nav_widget_extra_info_ =
                std::format("{},{}: {}", document_position.x, document_position.y, element_text(hovered));

        // If imgui is dealing with the mouse, we do nothing and let imgui change the cursor.
        if (ImGui::GetIO().WantCaptureMouse) {
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
            return;
        }

        // Otherwise we tell imgui not to mess with the cursor, and change it according to what we're
        // currently hovering over.
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        bool is_uri = try_get_uri(hovered).has_value();
        if (is_uri) {
            cursor_ = sf::Cursor::createFromSystem(sf::Cursor::Type::Hand);
        } else {
            cursor_ = sf::Cursor::createFromSystem(sf::Cursor::Type::Arrow);
        }

        if (cursor_) {
            window_.setMouseCursor(*cursor_);
        } else {
            spdlog::warn("Unable to create cursor '{}'", is_uri ? "hand" : "arrow");
        }
    } else if (auto const *mouse_button_released = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (ImGui::GetIO().WantCaptureMouse || mouse_button_released->button != sf::Mouse::Button::Left) {
            return;
        }

        auto window_position = geom::Position{mouse_button_released->position.x, mouse_button_released->position.y};
        auto document_position = to_document_position(std::move(window_position));
        auto const *hovered = get_hovered_node(std::move(document_position));
        if (auto uri = try_get_uri(hovered); uri.has_value()) {
            url_buf_ = std::string{*uri};
            navigate();
        }
    } else if (auto const *mouse_scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (ImGui::GetIO().WantCaptureMouse || mouse_scroll->wheel != sf::Mouse::Wheel::Vertical) {
            return;
        }

        scroll(std::lround(mouse_scroll->delta) * kMouseWheelScrollFactor);
    }
}

int App::run() {
    while (window_.isOpen()) {
        step();
    }

    return 0;
}

void App::navigate() {
    window_.setIcon({16, 16}, kBrowserIcon.data());
    auto uri = [this] {
        if (maybe_page_) {
            spdlog::info("Completing '{}' with '{}'", url_buf_, (**maybe_page_).uri.uri);
            return uri::Uri::parse(url_buf_, (**maybe_page_).uri);
        }

        return uri::Uri::parse(url_buf_);
    }();

    if (!uri) {
        spdlog::error("Unable to parse '{}'", url_buf_);
        return;
    }

    spdlog::info("Navigating to '{}'", uri->uri);
    browse_history_.push(*uri);
    ongoing_loads_.clear();
    pending_loads_.clear();
    images_.clear();
    maybe_page_ = engine_.navigate(*std::move(uri), make_options());

    // Make sure the displayed url is still correct if we followed any redirects.
    if (maybe_page_) {
        url_buf_ = (**maybe_page_).uri.uri;
        on_page_loaded();
    } else {
        url_buf_ = maybe_page_.error().uri.uri;
        on_navigation_failure(maybe_page_.error().response.err);
    }
}

void App::navigate_back() {
    auto entry = browse_history_.previous();
    if (!entry) {
        return;
    }

    browse_history_.pop();
    url_buf_ = entry->uri;
    navigate();
}

void App::navigate_forward() {
    auto entry = browse_history_.next();
    if (!entry) {
        return;
    }

    url_buf_ = entry->uri;
    navigate();
}

void App::reload() {
    if (!maybe_page_) {
        return;
    }

    url_buf_ = (**maybe_page_).uri.uri;
    navigate();
}

void App::on_navigation_failure(protocol::ErrorCode err) {
    switch (err) {
        case protocol::ErrorCode::Unresolved: {
            nav_widget_extra_info_ = std::format("Unable to resolve endpoint for '{}'", url_buf_);
            spdlog::error(nav_widget_extra_info_);
            break;
        }
        case protocol::ErrorCode::Unhandled: {
            nav_widget_extra_info_ = std::format("Unhandled protocol for '{}'", url_buf_);
            spdlog::error(nav_widget_extra_info_);
            break;
        }
        case protocol::ErrorCode::InvalidResponse: {
            nav_widget_extra_info_ = std::format("Invalid response from '{}'", url_buf_);
            spdlog::error(nav_widget_extra_info_);
            break;
        }
        case protocol::ErrorCode::RedirectLimit: {
            nav_widget_extra_info_ = std::format("Redirect limit hit while loading '{}'", url_buf_);
            spdlog::error(nav_widget_extra_info_);
            break;
        }
    }
}

void App::on_page_loaded() {
    if (auto page_title = try_get_text_content(page().dom, "/html/head/title"sv)) {
        auto title = std::format("{} - {}", *page_title, browser_title_);
        window_.setTitle(sf::String::fromUtf8(title.begin(), title.end()));
    } else {
        window_.setTitle(browser_title_);
    }

    // TODO(robinlinden): Non-blocking load of favicon.
    // https://developer.mozilla.org/en-US/docs/Web/HTML/Attributes/rel#icon
    auto is_favicon_link = [](dom::Element const *v) {
        auto rel = v->attributes.find("rel");
        return rel != end(v->attributes) && rel->second == "icon" && v->attributes.contains("href");
    };

    auto links = dom::nodes_by_xpath(page().dom.html(), "/html/head/link");
    std::ranges::reverse(links);
    for (auto const &link : links) {
        if (!is_favicon_link(link)) {
            continue;
        }

        auto uri = uri::Uri::parse(link->attributes.at("href"), page().uri);
        if (!uri) {
            spdlog::warn("Unable to parse favicon uri '{}'", link->attributes.at("href"));
            continue;
        }

        auto icon = engine_.load(*uri).response;
        sf::Image favicon;
        if (!icon.has_value()) {
            spdlog::warn("Error loading favicon from '{}': {}", uri->uri, to_string(icon.error().err));
            continue;
        }

        if (!favicon.loadFromMemory(icon->body.data(), icon->body.size())) {
            spdlog::warn("Error parsing favicon data from '{}'", uri->uri);
            continue;
        }

        window_.setIcon({favicon.getSize().x, favicon.getSize().y}, favicon.getPixelsPtr());
        break;
    }

    if (load_images_) {
        start_loading_images();
    }

    on_layout_updated();
}

void App::on_layout_updated() {
    reset_scroll();
    nav_widget_extra_info_.clear();
}

layout::LayoutBox const *App::get_hovered_node(geom::Position document_position) const {
    if (!maybe_page_.has_value()) {
        return nullptr;
    }

    auto const &maybe_layout = page().layout;
    if (!maybe_layout.has_value()) {
        return nullptr;
    }

    return layout::box_at_position(*maybe_layout, document_position);
}

geom::Position App::to_document_position(geom::Position window_position) const {
    return {window_position.x / static_cast<int>(scale_),
            window_position.y / static_cast<int>(scale_) - scroll_offset_y_};
}

void App::reset_scroll() {
    canvas_->add_translation(0, -scroll_offset_y_);
    scroll_offset_y_ = 0;
}

void App::scroll(int pixels) {
    if (!maybe_page_.has_value()) {
        return;
    }

    auto const &maybe_layout = page().layout;
    if (!maybe_layout.has_value()) {
        return;
    }

    auto const &layout = *maybe_layout;
    // Don't allow scrolling if the entire page fits on the screen.
    if (std::cmp_greater(window_.getSize().y / scale_, layout.dimensions.margin_box().height)) {
        return;
    }

    // Don't allow overscroll in either direction.
    if (scroll_offset_y_ + pixels > 0) {
        pixels = -scroll_offset_y_;
    }

    int current_bottom_visible_y = static_cast<int>(window_.getSize().y / scale_) - scroll_offset_y_;
    int scrolled_bottom_visible_y = current_bottom_visible_y - pixels;
    if (scrolled_bottom_visible_y > layout.dimensions.margin_box().height) {
        pixels -= layout.dimensions.margin_box().height - scrolled_bottom_visible_y;
    }

    canvas_->add_translation(0, pixels);
    scroll_offset_y_ += pixels;
}

void App::run_overlay() {
    ImGui::SFML::Update(window_, clock_.restart());
}

void App::focus_url_input() {
    auto *window = ImGui::FindWindowByName("Navigation");
    ImGui::ActivateItemByID(window->GetID("Url"));
}

void App::run_nav_widget() {
    ImGui::SetNextWindowPos({0, 0}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({window_.getSize().x / 2.f, 0}, ImGuiCond_FirstUseEver);

    ImGui::Begin("Navigation");
    if (ImGui::InputText("Url", &url_buf_, ImGuiInputTextFlags_EnterReturnsTrue)) {
        ensure_has_scheme(url_buf_);
        navigate();
    }

    ImGui::SameLine();
    if (ImGui::Button("Search")) {
        url_buf_ = std::format(
                "https://www.ecosia.org/search?q={}", url::percent_encode(url_buf_, url::PercentEncodeSet::query));
        navigate();
    }

    ImGui::TextUnformatted(nav_widget_extra_info_.c_str());
    ImGui::End();
}

void App::run_debug_widget() {
    ImGui::TextUnformatted("Print");
    ImGui::BeginDisabled(!maybe_page_.has_value());

    if (ImGui::Button("Status line")) {
        auto const &status = [this] {
            if (maybe_page_) {
                return page().response.status_line;
            }

            return maybe_page_.error().response.status_line.value_or(protocol::StatusLine{});
        }();

        std::cout << "\nStatus line:\n"
                  << std::format("{} {} {}", status.version, status.status_code, status.reason) << '\n';
    }

    if (ImGui::Button("Response headers")) {
        std::cout << "\nResponse headers:\n" << protocol::to_string(page().response.headers) << '\n';
    }

    if (ImGui::Button("Response body")) {
        std::cout << "\nResponse body:\n" << page().response.body << '\n';
    }

    if (ImGui::Button("DOM")) {
        std::cout << "\nDOM:\n" << to_string(page().dom) << '\n';
    }

    if (ImGui::Button("Stylesheet")) {
        std::cout << "\nStylesheet:\n" << to_string(page().stylesheet) << '\n';
    }

    std::optional<layout::LayoutBox> const &layout =
            maybe_page_.transform(&engine::PageState::layout).value_or(std::nullopt);
    ImGui::BeginDisabled(layout == std::nullopt);

    if (ImGui::Button("Layout")) {
        assert(layout);
        std::cout << "\nLayout:\n" << to_string(*layout) << '\n';
    }

    ImGui::EndDisabled();

    ImGui::EndDisabled();

    ImGui::Checkbox("Render depth debug", &render_debug_);
    if (ImGui::Checkbox("Enable image support", &load_images_)) {
        if (load_images_ && maybe_page_) {
            start_loading_images();
        } else {
            ongoing_loads_.clear();
            pending_loads_.clear();
            images_.clear();
            if (maybe_page_) {
                engine_.relayout(page(), make_options());
                on_layout_updated();
            }
        }
    }

    if (ImGui::Checkbox("Enable JavaScript", &enable_js_)) {
        if (maybe_page_) {
            // Scripting affects the html parsing, so we actually
            // need to reload the page for now.
            reload();
        }
    }

    if (ImGui::BeginCombo("Render backend", selected_canvas_ == Canvas::OpenGL ? "OpenGL" : "SFML")) {
        if (ImGui::Selectable("SFML", selected_canvas_ == Canvas::Sfml)) {
            select_canvas(Canvas::Sfml);
        }
        if (ImGui::Selectable("OpenGL", selected_canvas_ == Canvas::OpenGL)) {
            select_canvas(Canvas::OpenGL);
        }
        ImGui::EndCombo();
    }
}

void App::render_layout() {
    assert(maybe_page_);

    auto const &layout = page().layout;
    if (layout == std::nullopt) {
        return;
    }

    if (render_debug_) {
        render::debug::render_layout_depth(*canvas_, *layout);
    } else {
        render::render_layout(*canvas_,
                *layout,
                std::optional{geom::Rect{0,
                        -scroll_offset_y_,
                        static_cast<int>(window_.getSize().x),
                        static_cast<int>(window_.getSize().y)}},
                [this](std::string_view id) -> std::optional<render::ImageView> {
                    auto it = images_.find(id);
                    if (it == end(images_)) {
                        return std::nullopt;
                    }

                    return render::ImageView{it->second.width, it->second.height, it->second.rgba_bytes};
                });
    }
}

void App::render_overlay() {
    ImGui::SFML::Render(window_);
}

void App::show_render_surface() {
    window_.display();
}

void App::select_canvas(Canvas canvas) {
    reset_scroll();
    selected_canvas_ = canvas;
    if (canvas == Canvas::Sfml) {
        canvas_ = std::make_unique<gfx::SfmlCanvas>(window_, static_cast<type::SfmlType &>(engine_.font_system()));
    } else {
        canvas_ = std::make_unique<gfx::OpenGLCanvas>();
    }
    canvas_->set_scale(scale_);
    auto [width, height] = window_.getSize();
    canvas_->set_viewport_size(width, height);
}

void App::start_loading_images() {
    if (auto const &layout = page().layout; layout.has_value()) {
        constexpr static auto kSupportedImageTypes = std::to_array<std::string_view>({".png"sv, ".jpg"sv, ".jpeg"sv});
        auto image_urls = collect_image_urls(*layout, kSupportedImageTypes);
        for (auto const &url : image_urls) {
            auto uri = uri::Uri::parse(std::string{url}, page().uri);
            if (!uri) {
                spdlog::warn("Unable to parse image uri '{}'", url);
                continue;
            }

            if (ongoing_loads_.size() >= kMaxConcurrentImageLoads) {
                spdlog::info("Concurrent image load limit reached, queueing '{}'", uri->uri);
                pending_loads_.emplace_back(std::move(*uri), std::string{url});
                continue;
            }

            ongoing_loads_.push_back(load_image(engine_, std::move(*uri), std::string{url}));
        }
    }
}

engine::Options App::make_options() const {
    return {
            .layout_width = static_cast<int>(window_.getSize().x / scale_),
            .viewport_height = static_cast<int>(window_.getSize().y / scale_),
            .dark_mode = os::is_dark_mode(),
            .enable_js = enable_js_,
    };
}

} // namespace browser::gui
