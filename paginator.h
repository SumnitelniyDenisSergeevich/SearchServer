#pragma once

#include "search_server.h"

#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end)
        : first_(begin)
        , last_(end)
        , size_(distance(first_, last_)) {
    }

    [[nodiscard]] inline Iterator begin() const noexcept {
        return first_;
    }

    [[nodiscard]] inline Iterator end() const noexcept {
        return last_;
    }

    [[nodiscard]] inline size_t size() const noexcept {
        return size_;
    }

private:
    Iterator first_, last_;
    size_t size_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        for (size_t left = distance(begin, end); left > 0;) {
            const size_t current_page_size = std::min(page_size, left);
            const Iterator current_page_end = next(begin, current_page_size);
            pages_.push_back({ begin, current_page_end });
            left -= current_page_size;
            begin = current_page_end;
        }
    }

    [[nodiscard]] inline auto begin() const noexcept {
        return pages_.begin();
    }

    [[nodiscard]] inline auto end() const noexcept {
        return pages_.end();
    }

    [[nodiscard]] inline size_t size() const noexcept {
        return pages_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range) {
    for (Iterator it = range.begin(); it != range.end(); ++it) {
        out << *it;
    }
    return out;
}

template <typename Container>
[[nodiscard]] auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

