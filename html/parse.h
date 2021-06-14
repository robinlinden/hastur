#ifndef HTML_PARSE_H_
#define HTML_PARSE_H_

#include "dom/dom.h"

#include <string_view>

namespace html {

dom::Node parse(std::string_view input);

} // namespace html

#endif
