#include "css/parse.h"

#include "css/parser.h"

namespace css {

std::vector<css::Rule> parse(std::string_view input) {
    return css::Parser{input}.parse_rules();
}

} // namespace css
