// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/opengl_canvas.h"

#include "gfx/color.h"
#include "gfx/icanvas.h"
#include "gfx/opengl_shader.h"

#include "geom/geom.h"

#include <glad/gl.h>

#include <array>
#include <cassert>
#include <string_view>
#include <utility>

namespace gfx {
namespace {
#include "gfx/basic_vertex_shader.h"
#include "gfx/rect_fragment_shader.h"

std::string_view const vertex_shader{reinterpret_cast<char const *>(gfx_basic_shader_vert), gfx_basic_shader_vert_len};
std::string_view const fragment_shader{reinterpret_cast<char const *>(gfx_rect_shader_frag), gfx_rect_shader_frag_len};
} // namespace

OpenGLCanvas::OpenGLCanvas()
    : border_shader_{[] {
          auto shader = OpenGLShader::create(vertex_shader, fragment_shader);
          assert(shader.has_value());
          return std::move(shader).value();
      }()} {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void OpenGLCanvas::set_viewport_size(int width, int height) {
    size_x_ = width;
    size_y_ = height;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1.0, 1.0);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void OpenGLCanvas::clear(Color c) {
    glClearColor(c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLCanvas::draw_rect(
        geom::Rect const &rect, Color const &color, Borders const &borders, Corners const &corners) {
    auto translated{rect.translated(translation_x_, translation_y_)};
    auto inner_rect{translated.scaled(scale_)};
    auto outer_rect{
            inner_rect.expanded({borders.left.size, borders.right.size, borders.top.size, borders.bottom.size})};

    static constexpr auto kToArr2 = [](int a, int b) {
        return std::array<float, 2>{static_cast<float>(a), static_cast<float>(b)};
    };

    static constexpr auto kToColorArr = [](gfx::Color c) {
        return std::array<float, 4>{static_cast<float>(c.r) / 255.f,
                static_cast<float>(c.g) / 255.f,
                static_cast<float>(c.b) / 255.f,
                static_cast<float>(c.a) / 255.f};
    };

    border_shader_.enable();
    border_shader_.set_uniform("resolution", kToArr2(size_x_, size_y_));

    border_shader_.set_uniform("inner_top_left", kToArr2(inner_rect.left(), inner_rect.top()));
    border_shader_.set_uniform("inner_top_right", kToArr2(inner_rect.right(), inner_rect.top()));
    border_shader_.set_uniform("inner_bottom_left", kToArr2(inner_rect.left(), inner_rect.bottom()));
    border_shader_.set_uniform("inner_bottom_right", kToArr2(inner_rect.right(), inner_rect.bottom()));

    border_shader_.set_uniform("outer_top_left", kToArr2(outer_rect.left(), outer_rect.top()));
    border_shader_.set_uniform("outer_top_right", kToArr2(outer_rect.right(), outer_rect.top()));
    border_shader_.set_uniform("outer_bottom_left", kToArr2(outer_rect.left(), outer_rect.bottom()));
    border_shader_.set_uniform("outer_bottom_right", kToArr2(outer_rect.right(), outer_rect.bottom()));

    border_shader_.set_uniform("top_left_radii", kToArr2(corners.top_left.horizontal, corners.top_left.vertical));
    border_shader_.set_uniform("top_right_radii", kToArr2(corners.top_right.horizontal, corners.top_right.vertical));
    border_shader_.set_uniform(
            "bottom_left_radii", kToArr2(corners.bottom_left.horizontal, corners.bottom_left.vertical));
    border_shader_.set_uniform(
            "bottom_right_radii", kToArr2(corners.bottom_right.horizontal, corners.bottom_right.vertical));

    border_shader_.set_uniform("left_border_color", kToColorArr(borders.left.color));
    border_shader_.set_uniform("right_border_color", kToColorArr(borders.right.color));
    border_shader_.set_uniform("top_border_color", kToColorArr(borders.top.color));
    border_shader_.set_uniform("bottom_border_color", kToColorArr(borders.bottom.color));
    border_shader_.set_uniform("inner_rect_color", kToColorArr(color));

    glRecti(outer_rect.left(), outer_rect.top(), outer_rect.right(), outer_rect.bottom());
    border_shader_.disable();
}

} // namespace gfx
