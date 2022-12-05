#pragma once

#include <vector>

#include "document.h"

// contains documents for a single page
class IteratorRange {
public:
    template <typename Iterator>
    IteratorRange(Iterator page_begin, Iterator page_end) {
        for (Iterator iterator = page_begin; iterator != page_end; ) {
            docs_.push_back(*iterator);
            advance(iterator, 1);
        }
    }

    auto begin() const {
        return docs_.begin();
    }

    auto end() const {
        return docs_.end();
    }

    size_t size() const {
        return docs_.size();
    }

private:
    std::vector<Document> docs_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator range_begin, Iterator range_end, size_t page_size) {
        for (Iterator iterator = range_begin; iterator != range_end; ) {
            const size_t current_page_size = std::distance(iterator, range_end);
            if (current_page_size >= page_size) {
                pages_.push_back(IteratorRange(iterator, std::next(iterator, page_size)));
                std::advance(iterator, page_size);
            }
            else {
                pages_.push_back(IteratorRange(iterator, std::next(iterator, current_page_size)));
                std::advance(iterator, current_page_size);
            }
        }
    }

    template <typename Container>
    Iterator begin(Container c) {
        return c.begin();
    }

    template <typename Container>
    Iterator end(Container c) {
        return c.end();
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    size_t size() const {
        return pages_.size();
    }
private:
    std::vector<IteratorRange> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(std::begin(c), std::end(c), page_size);
}

std::ostream& operator<<(std::ostream& os, const IteratorRange& page) {
    for (auto iterator = page.begin(); iterator != page.end(); advance(iterator, 1)) {
        os << *iterator;
    }
    return os;
}