#ifndef TUI_TUI_H_
#define TUI_TUI_H_

#include "dom/dom.h"

#include <string>

namespace tui {

std::string render(dom::Node const &root);

} // namespace tui

#endif
