// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/default.h"

#include "css/parse.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace css {

std::vector<css::Rule> default_style() {
    auto path = std::filesystem::path("css/default.css");
    if (!exists(path) || !is_regular_file(path)) {
        return {};
    }

    auto file = std::ifstream(path, std::ios::in | std::ios::binary);
    auto size = file_size(path);
    auto content = std::string(size, '\0');
    file.read(content.data(), size);
    return css::parse(content);
}

} // namespace css
