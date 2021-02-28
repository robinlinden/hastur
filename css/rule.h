#ifndef CSS_RULE_H_
#define CSS_RULE_H_

#include <string>
#include <map>
#include <vector>

namespace css {

struct Rule {
    std::vector<std::string> selectors;
    std::map<std::string, std::string> declarations;
};

} // namespace css

#endif
