#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <set>

template <typename StringContainer>
[[nodiscard]] std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string_view& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(static_cast<std::string>(str));
        }
    }
    return non_empty_strings;
}

[[nodiscard]] std::vector<std::string_view> SplitIntoWords(std::string_view str);
