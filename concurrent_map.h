#pragma once

#include <mutex>
#include <map>

using namespace std::literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);
    struct Map_Mut {
        std::map<Key, Value> peace_of_full_map_;
        std::mutex m;
    };

    struct Access {
        Access(const Key& key, Map_Mut& m_m) : mut(std::lock_guard(m_m.m)), ref_to_value(m_m.peace_of_full_map_[key]) {

        }
        std::lock_guard<std::mutex> mut;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) :peaces_of_full_map_(bucket_count), bucket_count_(bucket_count) {

    };

    Access operator[](const Key& key) {
        return { key, peaces_of_full_map_[key % bucket_count_] };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& peace : peaces_of_full_map_) {
            std::lock_guard l_g(peace.m);
            result.insert(peace.peace_of_full_map_.begin(), peace.peace_of_full_map_.end());
        }
        return result;
    }

private:
    std::vector<Map_Mut> peaces_of_full_map_;
    size_t bucket_count_;
};