// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/gui/app.h"

#include "css/rule.h"
#include "css/style_sheet.h"
#include "dom/dom.h"
#include "dom/xpath.h"
#include "engine/engine.h"
#include "geom/geom.h"
#include "gfx/color.h"
#include "gfx/opengl_canvas.h"
#include "gfx/sfml_canvas.h"
#include "layout/layout_box.h"
#include "os/system_info.h"
#include "protocol/handler_factory.h"
#include "protocol/response.h"
#include "render/render.h"
#include "type/sfml.h"
#include "type/type.h"
#include "uri/uri.h"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Window/Cursor.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
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
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <variant>

using namespace std::literals;

namespace browser::gui {
namespace {

auto constexpr kDefaultResolutionX = 1024;
auto constexpr kDefaultResolutionY = 768;

// Magic number that felt right during testing.
auto constexpr kMouseWheelScrollFactor = 10;

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
    if (!url.contains("://")) {
        spdlog::info("Url missing scheme, assuming https");
        url = fmt::format("https://{}", url);
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

std::string stylesheet_to_string(css::StyleSheet const &stylesheet) {
    std::stringstream ss;
    for (auto const &rule : stylesheet.rules) {
        ss << css::to_string(rule) << '\n';
    }
    return std::move(ss).str();
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

namespace im {
void window(char const *title, ImVec2 const &position, ImVec2 const &size, auto content) {
    ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
    ImGui::Begin(title, nullptr, ImGuiWindowFlags_HorizontalScrollbar);
    content();
    ImGui::End();
}
} // namespace im

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

        spdlog::warn(
                "Unable to find a font for '{}', looked for [{}], good luck", name, fmt::join(file_name_options, ", "));
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

} // namespace

// Latest Firefox ESR user agent (on Windows). This matches what the Tor browser does.
App::App(std::string browser_title, std::string start_page_hint, bool load_start_page)
    : engine_{protocol::HandlerFactory::create(
                      "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:102.0) Gecko/20100101 Firefox/102.0"),
              create_font_system()},
      browser_title_{std::move(browser_title)},
      window_{sf::VideoMode(kDefaultResolutionX, kDefaultResolutionY), browser_title_},
      url_buf_{std::move(start_page_hint)},
      canvas_{std::make_unique<gfx::SfmlCanvas>(window_, static_cast<type::SfmlType &>(engine_.font_system()))} {
    window_.setMouseCursor(cursor_);
    window_.setIcon(16, 16, kBrowserIcon.data());
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
    sf::Event event{};
    while (window_.pollEvent(event)) {
        // ImGui needs a few iterations to do what it wants to do. This was
        // pretty much picked at random after I still occasionally got
        // unexpected results when giving it 2 iterations.
        process_iterations_ = 5;
        ImGui::SFML::ProcessEvent(event);

        switch (event.type) {
            case sf::Event::Closed: {
                window_.close();
                break;
            }
            case sf::Event::Resized: {
                canvas_->set_viewport_size(event.size.width, event.size.height);
                if (maybe_page_) {
                    engine_.relayout(**maybe_page_, make_options());
                    on_layout_updated();
                }
                break;
            }
            case sf::Event::KeyPressed: {
                if (ImGui::GetIO().WantCaptureKeyboard) {
                    break;
                }

                switch (event.key.code) {
                    case sf::Keyboard::Key::J: {
                        scroll(event.key.shift ? -20 : -5);
                        break;
                    }
                    case sf::Keyboard::Key::K: {
                        scroll(event.key.shift ? 20 : 5);
                        break;
                    }
                    case sf::Keyboard::Key::L: {
                        if (!event.key.control) {
                            break;
                        }
                        focus_url_input();
                        break;
                    }
                    case sf::Keyboard::Key::F1: {
                        render_debug_ = !render_debug_;
                        spdlog::info("Render debug: {}", render_debug_);
                        break;
                    }
                    case sf::Keyboard::Key::F2: {
                        switch_canvas();
                        spdlog::info("Switched canvas to {}", selected_canvas_ == Canvas::OpenGL ? "OpenGL" : "SFML");
                        break;
                    }
                    case sf::Keyboard::Key::F3: {
                        culling_enabled_ = !culling_enabled_;
                        spdlog::info("Culling enabled: {}", culling_enabled_);
                        break;
                    }
                    case sf::Keyboard::Key::F4: {
                        display_debug_gui_ = !display_debug_gui_;
                        spdlog::info("Display debug gui: {}", display_debug_gui_);
                        break;
                    }
                    case sf::Keyboard::Key::Left: {
                        if (!event.key.alt) {
                            break;
                        }
                        navigate_back();
                        break;
                    }
                    case sf::Keyboard::Key::Right: {
                        if (!event.key.alt) {
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
                        if (!event.key.control) {
                            break;
                        }
                        reload();
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case sf::Event::MouseMoved: {
                if (!maybe_page_) {
                    break;
                }

                auto window_position = geom::Position{event.mouseMove.x, event.mouseMove.y};
                auto document_position = to_document_position(std::move(window_position));
                auto const *hovered = get_hovered_node(document_position);
                nav_widget_extra_info_ =
                        fmt::format("{},{}: {}", document_position.x, document_position.y, element_text(hovered));

                // If imgui is dealing with the mouse, we do nothing and let imgui change the cursor.
                if (ImGui::GetIO().WantCaptureMouse) {
                    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
                    break;
                }

                // Otherwise we tell imgui not to mess with the cursor, and change it according to what we're
                // currently hovering over.
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
                if (try_get_uri(hovered).has_value()) {
                    cursor_.loadFromSystem(sf::Cursor::Hand);
                } else {
                    cursor_.loadFromSystem(sf::Cursor::Arrow);
                }
                window_.setMouseCursor(cursor_);

                break;
            }
            case sf::Event::MouseButtonReleased: {
                if (ImGui::GetIO().WantCaptureMouse || event.mouseButton.button != sf::Mouse::Left) {
                    break;
                }

                auto window_position = geom::Position{event.mouseButton.x, event.mouseButton.y};
                auto document_position = to_document_position(std::move(window_position));
                auto const *hovered = get_hovered_node(std::move(document_position));
                if (auto uri = try_get_uri(hovered); uri.has_value()) {
                    url_buf_ = std::string{*uri};
                    navigate();
                }

                break;
            }
            case sf::Event::MouseWheelScrolled: {
                if (ImGui::GetIO().WantCaptureMouse
                        || event.mouseWheelScroll.wheel != sf::Mouse::Wheel::VerticalWheel) {
                    break;
                }

                scroll(std::lround(event.mouseWheelScroll.delta) * kMouseWheelScrollFactor);
                break;
            }
            default:
                break;
        }
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
        run_http_response_widget();
        run_dom_widget();
        run_stylesheet_widget();
        run_layout_widget();
    }

    if (!maybe_page_ || (**maybe_page_).layout == std::nullopt) {
        canvas_->clear(gfx::Color{255, 255, 255});
    } else {
        render_layout();
    }

    render_overlay();
    show_render_surface();
}

int App::run() {
    while (window_.isOpen()) {
        step();
    }

    return 0;
}

void App::navigate() {
    spdlog::info("Navigating to '{}'", url_buf_);
    window_.setIcon(16, 16, kBrowserIcon.data());
    auto uri = [this] {
        if (maybe_page_) {
            return uri::Uri::parse(url_buf_, (**maybe_page_).uri);
        }

        return uri::Uri::parse(url_buf_);
    }();

    if (!uri) {
        spdlog::error("Unable to parse '{}'", url_buf_);
        return;
    }

    browse_history_.push(*uri);
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
    update_status_line();
    response_headers_str_.clear();
    dom_str_.clear();
    stylesheet_str_.clear();
    layout_str_.clear();

    switch (err) {
        case protocol::ErrorCode::Unresolved: {
            nav_widget_extra_info_ = fmt::format("Unable to resolve endpoint for '{}'", url_buf_);
            spdlog::error(nav_widget_extra_info_);
            break;
        }
        case protocol::ErrorCode::Unhandled: {
            nav_widget_extra_info_ = fmt::format("Unhandled protocol for '{}'", url_buf_);
            spdlog::error(nav_widget_extra_info_);
            break;
        }
        case protocol::ErrorCode::InvalidResponse: {
            nav_widget_extra_info_ = fmt::format("Invalid response from '{}'", url_buf_);
            spdlog::error(nav_widget_extra_info_);
            break;
        }
        case protocol::ErrorCode::RedirectLimit: {
            nav_widget_extra_info_ = fmt::format("Redirect limit hit while loading '{}'", url_buf_);
            spdlog::error(nav_widget_extra_info_);
            break;
        }
    }
}

void App::on_page_loaded() {
    if (auto page_title = try_get_text_content(page().dom, "/html/head/title"sv)) {
        auto title = fmt::format("{} - {}", *page_title, browser_title_);
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

        window_.setIcon(favicon.getSize().x, favicon.getSize().y, favicon.getPixelsPtr());
        break;
    }

    update_status_line();
    response_headers_str_ = page().response.headers.to_string();
    dom_str_ = dom::to_string(page().dom);
    stylesheet_str_ = stylesheet_to_string(page().stylesheet);
    on_layout_updated();
}

void App::on_layout_updated() {
    reset_scroll();
    nav_widget_extra_info_.clear();
    if (!maybe_page_) {
        return;
    }

    auto const &layout = page().layout;
    layout_str_ = layout.has_value() ? layout::to_string(*layout) : "";
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
    if (static_cast<int>(window_.getSize().y / scale_) > layout.dimensions.margin_box().height) {
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

void App::update_status_line() {
    auto const &status = [this] {
        if (maybe_page_) {
            return page().response.status_line;
        }

        return maybe_page_.error().response.status_line.value_or(protocol::StatusLine{});
    }();

    status_line_str_ = fmt::format("{} {} {}", status.version, status.status_code, status.reason);
}

void App::run_overlay() {
    ImGui::SFML::Update(window_, clock_.restart());
}

void App::focus_url_input() {
    auto *window = ImGui::FindWindowByName("Navigation");
    ImGui::ActivateItemByID(window->GetID("Url"));
}

void App::run_nav_widget() {
    im::window("Navigation", {0, 0}, {window_.getSize().x / 2.f, 0}, [this] {
        if (ImGui::InputText("Url", &url_buf_, ImGuiInputTextFlags_EnterReturnsTrue)) {
            ensure_has_scheme(url_buf_);
            navigate();
        }
        ImGui::TextUnformatted(nav_widget_extra_info_.c_str());
    });
}

void App::run_http_response_widget() const {
    auto const &size = window_.getSize();
    im::window("HTTP Response", {size.x / 2.f, 0}, {size.x / 2.f, size.y / 2.f}, [this] {
        ImGui::TextUnformatted(status_line_str_.c_str());
        if (ImGui::CollapsingHeader("Headers")) {
            ImGui::TextUnformatted(response_headers_str_.c_str());
        }

        if (ImGui::CollapsingHeader("Body")) {
            if (maybe_page_) {
                ImGui::TextUnformatted(page().response.body.c_str());
            }
        }
    });
}

void App::run_dom_widget() const {
    im::window("DOM", {0, 70.f * scale_}, {window_.getSize().x / 2.f, window_.getSize().y / 2.f}, [this] {
        ImGui::TextUnformatted(dom_str_.c_str());
    });
}

void App::run_stylesheet_widget() const {
    auto const &size = window_.getSize();
    im::window("Stylesheet", {0, 70.f * scale_ + size.y / 2.f}, {size.x / 2.f, size.y / 2.f}, [this] {
        ImGui::TextUnformatted(stylesheet_str_.c_str());
    });
}

void App::run_layout_widget() const {
    auto const &size = window_.getSize();
    im::window("Layout", {size.x / 2.f, size.y / 2.f}, {size.x / 2.f, size.y / 2.f}, [this] {
        ImGui::TextUnformatted(layout_str_.c_str());
    });
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
                culling_enabled_ ? std::optional{geom::Rect{0,
                                           -scroll_offset_y_,
                                           static_cast<int>(window_.getSize().x),
                                           static_cast<int>(window_.getSize().y)}}
                                 : std::nullopt);
    }
}

void App::render_overlay() {
    ImGui::SFML::Render(window_);
}

void App::show_render_surface() {
    window_.display();
}

void App::switch_canvas() {
    reset_scroll();
    if (selected_canvas_ == Canvas::OpenGL) {
        selected_canvas_ = Canvas::Sfml;
        canvas_ = std::make_unique<gfx::SfmlCanvas>(window_, static_cast<type::SfmlType &>(engine_.font_system()));
    } else {
        selected_canvas_ = Canvas::OpenGL;
        canvas_ = std::make_unique<gfx::OpenGLCanvas>();
    }
    canvas_->set_scale(scale_);
    auto [width, height] = window_.getSize();
    canvas_->set_viewport_size(width, height);
}

engine::Options App::make_options() const {
    return {
            .layout_width = static_cast<int>(window_.getSize().x / scale_),
            .dark_mode = os::is_dark_mode(),
    };
}

} // namespace browser::gui
