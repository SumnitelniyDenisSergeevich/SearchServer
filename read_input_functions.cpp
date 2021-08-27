#include "read_input_functions.h"
#include <iostream>

std::string ReadLine() {
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    (void)ReadLine();
    return result;
}