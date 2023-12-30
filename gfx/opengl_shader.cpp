// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/opengl_shader.h"

#include <glad/gl.h>

#include <cassert>
#include <cstdlib>
#include <optional>
#include <span>
#include <string_view>

// NOLINTBEGIN(readability-make-member-function-const): The drawing code
// shouldn't be thought of as const, even if it technically doesn't modify any
// class members.
namespace gfx {

std::optional<OpenGLShader> OpenGLShader::create(std::string_view vertex_src, std::string_view fragment_src) {
    // TODO(robinlinden): Move this somewhere more sensible. I think calling it
    // multiple times is fine, so it living here for now probably won't cause
    // issues.
    if (gladLoaderLoadGL() == 0) {
        std::abort();
    }

    GLint success{0};
    GLint shader_length{0};
    GLchar const *shader_src{nullptr};

    shader_length = static_cast<GLint>(vertex_src.length());
    shader_src = vertex_src.data();
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &shader_src, &shader_length);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        glDeleteShader(vertex_shader);
        return std::nullopt;
    }

    shader_length = static_cast<GLint>(fragment_src.length());
    shader_src = fragment_src.data();
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &shader_src, &shader_length);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        glDeleteShader(fragment_shader);
        glDeleteShader(vertex_shader);
        return std::nullopt;
    }

    GLuint program = glCreateProgram();
    if (program == 0) {
        glDeleteShader(fragment_shader);
        glDeleteShader(vertex_shader);
        return std::nullopt;
    }

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        glDeleteProgram(program);
        glDeleteShader(fragment_shader);
        glDeleteShader(vertex_shader);
        return std::nullopt;
    }

    glDetachShader(program, fragment_shader);
    glDetachShader(program, vertex_shader);
    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);

    return OpenGLShader{program};
}

OpenGLShader::~OpenGLShader() {
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
}

void OpenGLShader::enable() {
    glUseProgram(program_);
}

void OpenGLShader::disable() {
    glUseProgram(0);
}

void OpenGLShader::set_uniform(char const *name, std::span<float const, 2> data) {
    auto loc = glGetUniformLocation(program_, name);
    assert(loc != -1);
    glUniform2f(loc, data[0], data[1]);
}

void OpenGLShader::set_uniform(char const *name, std::span<float const, 4> data) {
    auto loc = glGetUniformLocation(program_, name);
    assert(loc != -1);
    glUniform4f(loc, data[0], data[1], data[2], data[3]);
}

} // namespace gfx
// NOLINTEND(readability-make-member-function-const)
