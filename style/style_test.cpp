#include "style/style.h"

#include "css/rule.h"
#include "etest/etest.h"

using namespace std::literals;
using etest::expect_true;

int main() {
    etest::test("is_match: simple names", [] {
        expect_true(style::is_match(dom::Element{"div"}, "div"sv));
        expect_true(!style::is_match(dom::Element{"div"}, "span"sv));
    });

    etest::test("matching_rules: simple names", [] {
        std::vector<css::Rule> stylesheet;
        expect_true(style::matching_rules(dom::Element{"div"}, stylesheet).empty());

        stylesheet.push_back(css::Rule{
            .selectors = {"span", "p"},
            .declarations = {
                {"width", "80px"},
            }
        });

        expect_true(style::matching_rules(dom::Element{"div"}, stylesheet).empty());

        {
            auto span_rules = style::matching_rules(dom::Element{"span"}, stylesheet);
            expect_true(span_rules.size() == 1);
            expect_true(span_rules.at(0) == std::pair{"width"s, "80px"s});
        }

        {
            auto p_rules = style::matching_rules(dom::Element{"p"}, stylesheet);
            expect_true(p_rules.size() == 1);
            expect_true(p_rules.at(0) == std::pair{"width"s, "80px"s});
        }

        stylesheet.push_back(css::Rule{
            .selectors = {"span", "hr"},
            .declarations = {
                {"height", "auto"},
            }
        });

        expect_true(style::matching_rules(dom::Element{"div"}, stylesheet).empty());

        {
            auto span_rules = style::matching_rules(dom::Element{"span"}, stylesheet);
            expect_true(span_rules.size() == 2);
            expect_true(span_rules.at(0) == std::pair{"width"s, "80px"s});
            expect_true(span_rules.at(1) == std::pair{"height"s, "auto"s});
        }

        {
            auto p_rules = style::matching_rules(dom::Element{"p"}, stylesheet);
            expect_true(p_rules.size() == 1);
            expect_true(p_rules.at(0) == std::pair{"width"s, "80px"s});
        }

        {
            auto hr_rules = style::matching_rules(dom::Element{"hr"}, stylesheet);
            expect_true(hr_rules.size() == 1);
            expect_true(hr_rules.at(0) == std::pair{"height"s, "auto"s});
        }
    });

    return etest::run_all_tests();
}
