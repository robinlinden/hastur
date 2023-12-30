// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_HISTORY_H_
#define UTIL_HISTORY_H_

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace util {

template<typename T>
class History {
public:
    constexpr void push(T entry) {
        // Are we already on this entry?
        if (current_index_ >= 0 && entries_[current_index_] == entry) {
            return;
        }

        current_index_ += 1;

        // Does the entry already exist in the history where we want it to be?
        if (static_cast<std::size_t>(current_index_) < entries_.size() && entries_[current_index_] == entry) {
            return;
        }

        // Does the entry require more space in the list?
        if (static_cast<std::size_t>(current_index_) == entries_.size()) {
            entries_.push_back(std::move(entry));
            return;
        }

        // This entry should go in the middle of the history. Add it and nuke everything after it.
        entries_[current_index_] = std::move(entry);
        entries_.erase(cbegin(entries_) + current_index_ + 1, cend(entries_));
    }

    constexpr std::optional<T> pop() {
        if (current_index_ >= 0) {
            return entries_[std::exchange(current_index_, current_index_ - 1)];
        }

        return std::nullopt;
    }

    constexpr std::optional<T> previous() const {
        if (auto previous_index = current_index_ - 1; previous_index >= 0) {
            return entries_[previous_index];
        }

        return std::nullopt;
    }

    constexpr std::optional<T> current() const {
        if (current_index_ >= 0 && static_cast<std::size_t>(current_index_) < entries_.size()) {
            return entries_[current_index_];
        }

        return std::nullopt;
    }

    constexpr std::optional<T> next() const {
        if (auto next_index = static_cast<std::size_t>(current_index_) + 1; next_index < entries_.size()) {
            return entries_[next_index];
        }

        return std::nullopt;
    }

    constexpr std::vector<T> const &entries() const { return entries_; }

private:
    int current_index_{-1};
    std::vector<T> entries_;
};

} // namespace util

#endif
