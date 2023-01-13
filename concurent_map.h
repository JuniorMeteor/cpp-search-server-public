#pragma once

#include <string>
#include <mutex>
#include <vector>
#include <map>

template <typename Key, typename Value>
class ConcurrentMap {

    using namespace std::string_literals;

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) : size_(bucket_count) {
        vec_mutex_ = std::vector<std::mutex>(size_);
        vec_bucket_.resize(size_);
    }

    Access operator[](const Key& key) {
        uint64_t bucket = key % size_;
        return { std::lock_guard(vec_mutex_[bucket]), vec_bucket_[bucket][key] };
    }

    std::map<Key, Value> ConcurrentMap::BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (size_t i = 0; i < size_; ++i) {
            std::lock_guard<std::mutex> guard(vec_mutex_[i]);
            result.insert(vec_bucket_[i].begin(), vec_bucket_[i].end());
        }
        return result;
    }

private:
    std::vector<std::mutex> vec_mutex_;
    std::vector<std::map<Key, Value>> vec_bucket_;
    size_t size_;
};