#include "os/os.h"

#include <cstdlib>

using namespace std::literals;

namespace os {

std::vector<std::string> font_paths() {
    std::vector<std::string> paths{};
    if (char const *xdg_data_home = std::getenv("XDG_DATA_HOME")) {
        paths.push_back(xdg_data_home + "/fonts"s);
    }

    if (char const *home = std::getenv("HOME")) {
        if (paths.empty()) {
            paths.push_back(home + "/.local/share/fonts"s);
        }
        paths.push_back(home + "/.fonts"s);
    }

    paths.push_back("/usr/share/fonts"s);
    paths.push_back("/usr/local/share/fonts"s);
    return paths;
}

} // namespace os
