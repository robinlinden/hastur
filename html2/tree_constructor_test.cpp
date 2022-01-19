// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/tree_constructor.h"

#include "dom2/document_type.h"
#include "etest/etest.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace std::literals;

using etest::expect_eq;
using etest::require;

using namespace html2;

namespace {

std::unique_ptr<dom2::Document> construct_from(std::vector<html2::Token> tokens) {
    auto constructor = TreeConstructor{};
    constructor.run(std::move(tokens));
    return constructor.take_document();
}

} // namespace

int main() {
    etest::test("document with only doctype", [] {
        auto document = construct_from({DoctypeToken{"html"s}});

        expect_eq(document->type(), dom2::NodeType::Document);
        expect_eq(document->child_nodes().size(), size_t{1});

        expect_eq(document->first_child()->type(), dom2::NodeType::DocumentType);
        auto const *doctype = static_cast<dom2::DocumentType const *>(document->first_child());
        expect_eq(doctype->name(), "html"s);
    });

    return etest::run_all_tests();
}
