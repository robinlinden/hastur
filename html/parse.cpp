#include "html/parse.h"

#include "html/parser.h"

namespace html {

dom::Document parse(std::string_view input) {
    return Parser{input}.parse_document();
}

} // namespace html
