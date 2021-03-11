#include "style/style.h"

#include "css/rule.h"
#include "etest/etest.h"

using namespace std::literals;
using etest::expect;

int main() {
    etest::test("is_match: simple names", [] {
        expect(style::is_match(dom::Element{"div"}, "div"sv));
        expect(!style::is_match(dom::Element{"div"}, "span"sv));
    });

    etest::test("matching_rules: simple names", [] {
        std::vector<css::Rule> stylesheet;
        expect(style::matching_rules(dom::Element{"div"}, stylesheet).empty());

        stylesheet.push_back(css::Rule{
            .selectors = {"span", "p"},
            .declarations = {
                {"width", "80px"},
            }
        });

        expect(style::matching_rules(dom::Element{"div"}, stylesheet).empty());

        {
            auto span_rules = style::matching_rules(dom::Element{"span"}, stylesheet);
            expect(span_rules.size() == 1);
            expect(span_rules.at(0) == std::pair{"width"s, "80px"s});
        }

        {
            auto p_rules = style::matching_rules(dom::Element{"p"}, stylesheet);
            expect(p_rules.size() == 1);
            expect(p_rules.at(0) == std::pair{"width"s, "80px"s});
        }

        stylesheet.push_back(css::Rule{
            .selectors = {"span", "hr"},
            .declarations = {
                {"height", "auto"},
            }
        });

        expect(style::matching_rules(dom::Element{"div"}, stylesheet).empty());

        {
            auto span_rules = style::matching_rules(dom::Element{"span"}, stylesheet);
            expect(span_rules.size() == 2);
            expect(span_rules.at(0) == std::pair{"width"s, "80px"s});
            expect(span_rules.at(1) == std::pair{"height"s, "auto"s});
        }

        {
            auto p_rules = style::matching_rules(dom::Element{"p"}, stylesheet);
            expect(p_rules.size() == 1);
            expect(p_rules.at(0) == std::pair{"width"s, "80px"s});
        }

        {
            auto hr_rules = style::matching_rules(dom::Element{"hr"}, stylesheet);
            expect(hr_rules.size() == 1);
            expect(hr_rules.at(0) == std::pair{"height"s, "auto"s});
        }
    });

    return etest::run_all_tests();
}
