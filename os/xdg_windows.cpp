// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "os/xdg.h"

#include "os/windows_setup.h"

#include <Knownfolders.h>
#include <Objbase.h>
#include <Shlobj.h>

#include <cwchar>

namespace os {

std::vector<std::string> font_paths() {
    PWSTR bad_font_path{nullptr};
    SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &bad_font_path);
    auto bad_font_path_len = static_cast<int>(std::wcslen(bad_font_path));
    auto chars_needed = WideCharToMultiByte(CP_UTF8, 0, bad_font_path, bad_font_path_len, nullptr, 0, nullptr, nullptr);
    std::string font_path;
    font_path.resize(chars_needed);
    WideCharToMultiByte(CP_UTF8, 0, bad_font_path, bad_font_path_len, font_path.data(), chars_needed, nullptr, nullptr);
    CoTaskMemFree(bad_font_path);
    return {font_path};
}

} // namespace os
