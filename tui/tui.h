#ifndef TUI_TUI_H_
#define TUI_TUI_H_

#include "layout/layout.h"

#include <string>

namespace tui {

std::string render(layout::LayoutBox const &root);

} // namespace tui

#endif
