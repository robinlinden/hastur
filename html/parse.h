#ifndef HTML_PARSE_H_
#define HTML_PARSE_H_

#include "dom/dom.h"

#include <string_view>
#include <vector>

namespace html {

std::vector<dom::Node> parse(std::string_view input);

} // namespace html

#endif
