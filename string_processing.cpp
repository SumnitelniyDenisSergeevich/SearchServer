#include "string_processing.h"

#include <iostream>

std::string ReadLine() {
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}

std::vector < std::string_view > SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;
    const int64_t pos_end = str.npos;
    while (true) {
        int64_t space = str.find(' ');
        result.push_back(space == pos_end ? str.substr(0) : str.substr(0, space));
        if (space == pos_end) {
            break;
        }
        else {
            str.remove_prefix(static_cast<int64_t>(space) + 1);
        }
    }
    return result;
}
