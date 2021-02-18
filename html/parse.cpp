#include "html/parse.h"

#include "html/parser.h"

namespace html {

std::vector<dom::Node> parse(std::string_view input) {
    return Parser{input}.parse_nodes();
}

} // namespace html
