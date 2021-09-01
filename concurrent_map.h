#pragma once

#include <mutex>
#include <map>

using namespace std::literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Map_Mutex {
        std::map<Key, Value> peace_of_full_map_;
        std::mutex changes_in_map;
    };

    struct Access {
        Access(const Key& key, Map_Mutex& map_mutex) : map_synchronizer(std::lock_guard(map_mutex.changes_in_map)), ref_to_value(map_mutex.peace_of_full_map_[key]) {
        }
        std::lock_guard<std::mutex> map_synchronizer;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) :peaces_of_full_map_(bucket_count) {

    };

    Access operator[](const Key& key) {
        return { key, peaces_of_full_map_[key % peaces_of_full_map_.size()] };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& peace : peaces_of_full_map_) {
            std::lock_guard l_g(peace.changes_in_map);
            result.insert(peace.peace_of_full_map_.begin(), peace.peace_of_full_map_.end());
        }
        return result;
    }

private:
    std::vector<Map_Mutex> peaces_of_full_map_;
};