// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM_XPATH_H_
#define DOM_XPATH_H_

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string_view>
#include <vector>

namespace dom {

// https://developer.mozilla.org/en-US/docs/Web/XPath
// https://en.wikipedia.org/wiki/XPath
template<typename T>
inline std::vector<T const *> nodes_by_xpath(T const &root, std::string_view xpath) {
    std::vector<T const *> next_search{&root};
    std::vector<T const *> searching{};
    std::vector<T const *> goal_nodes{};

    // We only support xpaths in the form /a/b/c right now.
    if (!xpath.starts_with('/')) {
        return {};
    }

    static constexpr std::string_view kSeparators{"|/"};
    auto is_separator = [](char c) {
        return kSeparators.contains(c);
    };

    auto remove_name_segment = [&] {
        std::size_t separator_position{xpath.find_first_of(kSeparators)};
        if (separator_position == std::string_view::npos) {
            xpath = std::string_view{};
            return;
        }
        xpath.remove_prefix(separator_position);
    };

    auto search_children = [&] {
        xpath.remove_prefix(1);
        for (auto const *node : searching) {
            auto name = dom_name(*node);
            if (xpath.substr(0, xpath.find_first_of('|')) == name) {
                goal_nodes.push_back(node);
                continue;
            }

            if (xpath.starts_with(name) && xpath.size() >= name.size() + 1 && is_separator(xpath[name.size()])) {
                for (auto const *child : dom_children(*node)) {
                    next_search.push_back(child);
                }
            }
        }
    };

    auto search_descendants = [&] {
        xpath.remove_prefix(2);
        for (std::size_t i = 0; i < searching.size(); ++i) {
            auto const *node = searching[i];

            auto name = dom_name(*node);
            if (xpath.substr(0, xpath.find_first_of('|')) == name) {
                // TODO(robinlinden): Less terrible way of deduplicating goal nodes.
                if (std::ranges::find(goal_nodes, node) == end(goal_nodes)) {
                    goal_nodes.push_back(node);
                }
            } else if (xpath.starts_with(name) && xpath.size() >= name.size() + 1 && is_separator(xpath[name.size()])) {
                std::ranges::move(dom_children(*node), std::back_inserter(next_search));
            }

            // Pretty gross, but we want to perform the search in tree order.
            std::ranges::move(dom_children(*node), std::insert_iterator(searching, next(begin(searching), i + 1)));
        }
    };

    while (!next_search.empty() && !xpath.empty()) {
        searching.swap(next_search);
        next_search.clear();
        if (xpath.starts_with("//")) {
            search_descendants();
        } else if (xpath.starts_with('/')) {
            search_children();
        }
        remove_name_segment();

        if (xpath.starts_with('|')) {
            next_search = {&root};
            xpath.remove_prefix(1);
        }
    }

    return goal_nodes;
}

} // namespace dom

#endif
