#ifndef RENDER_RENDER_H_
#define RENDER_RENDER_H_

#include "layout/layout.h"

namespace render {

void render_setup(int width, int height);
void render_layout(layout::LayoutBox const &layout, int scroll_offset);

} // namespace render

#endif
