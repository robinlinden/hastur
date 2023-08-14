// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_OPENGL_SHADER_H_
#define GFX_OPENGL_SHADER_H_

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

namespace gfx {

class OpenGLShader {
public:
    static std::optional<OpenGLShader> create(std::string_view vertex_src, std::string_view fragment_src);

    OpenGLShader(OpenGLShader const &) = delete;
    OpenGLShader &operator=(OpenGLShader const &) = delete;

    OpenGLShader(OpenGLShader &&o) noexcept : program_{std::exchange(o.program_, 0)} {}
    OpenGLShader &operator=(OpenGLShader &&o) noexcept {
        program_ = std::exchange(o.program_, 0);
        return *this;
    }

    ~OpenGLShader();

    void enable();
    void disable();
    void set_uniform(char const *name, std::span<float const, 2>);
    void set_uniform(char const *name, std::span<float const, 4>);

    std::uint32_t id() const { return program_; }

private:
    explicit OpenGLShader(std::uint32_t program) : program_{program} {}

    // Really a GLuint, but https://www.khronos.org/opengl/wiki/OpenGL_Type says
    // it's always exactly 32 bits wide, so let's hide the GL header properly.
    std::uint32_t program_{0};
};

} // namespace gfx

#endif
