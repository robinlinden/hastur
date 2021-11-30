// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_CHARACTER_DATA_H_
#define DOM2_CHARACTER_DATA_H_

#include "dom2/node.h"

#include <cstddef>
#include <exception>
#include <string>
#include <string_view>
#include <utility>

namespace dom2 {

// https://dom.spec.whatwg.org/#interface-characterdata
// TODO(robinlinden): The spec wants the strings to be 16-bit integers.
class CharacterData : public Node {
protected:
    explicit CharacterData(std::string data) : data_{std::move(data)} {}

public:
    std::string const &data() const { return data_; }
    std::size_t length() const { return data_.length(); }

    std::string substring_data(std::size_t offset, std::size_t count) const {
        if (offset > length()) {
            // TODO(robinlinden): then throw an "IndexSizeError" DOMException.
            std::terminate();
        }

        return data().substr(offset, count);
    }

    void append_data(std::string_view data) { replace_data(length(), 0, data); }
    void insert_data(std::size_t offset, std::string_view data) { replace_data(offset, 0, data); }
    void delete_data(std::size_t offset, std::size_t count) { replace_data(offset, count, std::string_view{""}); }

    // https://dom.spec.whatwg.org/#concept-cd-replace
    // TODO(robinlinden): Mutation record, live range, and children changed stuff.
    void replace_data(std::size_t offset, std::size_t count, std::string_view data) {
        // Let length be node's length.
        // If offset is greater than length, then throw an "IndexSizeError" DOMException.
        if (offset > length()) {
            // TODO(robinlinden): then throw an "IndexSizeError" DOMException.
            std::terminate();
        }

        // If offset plus count is greater than length, then set count to length minus offset.
        if (offset + count > length()) {
            count = length() - offset;
        }

        // Queue a mutation record of "characterData" for node with null, null,
        // node's data, « », « », null, and null.

        // Insert data into node's data after offset code units.
        data_.insert(offset, data);

        // Let delete offset be offset + data's length.
        std::size_t delete_offset = offset + data.length();

        // Starting from delete offset code units, remove count code units from node's data.
        data_.erase(delete_offset, count);

        // For each live range whose start node is node and start offset is
        // greater than offset but less than or equal to offset plus count, set
        // its start offset to offset.

        // For each live range whose end node is node and end offset is greater
        // than offset but less than or equal to offset plus count, set its end
        // offset to offset.

        // For each live range whose start node is node and start offset is
        // greater than offset plus count, increase its start offset by data's
        // length and decrease it by count.

        // For each live range whose end node is node and end offset is greater
        // than offset plus count, increase its end offset by data's length and
        // decrease it by count.

        // If node's parent is non-null, then run the children changed steps for node's parent.
    }

private:
    std::string data_{};
};

} // namespace dom2

#endif
