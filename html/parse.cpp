#include "html/parse.h"

#include "html/parser.h"

namespace html {

dom::Node parse(std::string_view input) {
    return Parser{input}.parse_document();
}

} // namespace html
