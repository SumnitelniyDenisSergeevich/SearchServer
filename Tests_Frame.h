#pragma once

#include <iostream>
#include <set>
#include <map>
#include <string>
#include <vector>


template <typename Type>
std::ostream& operator<<(std::ostream& out, const std::vector<Type> V) {
    out << std::string{ "[" };
    bool b = false;
    for (const Type& temp : V) {
        if (b) {
            out << std::string{ ", " };
        }
        b = true;
        out << temp;
    }
    out << std::string{ "]" };
    return out;
}

template <typename Type>
std::ostream& operator<<(std::ostream& out, const std::set<Type> S) {
    out << std::string{ "{" };
    bool b = false;
    for (const Type& temp : S) {
        if (b) {
            out << std::string{ ", " };
        }
        b = true;
        out << temp;
    }
    out << std::string{ "}" };
    return out;
}

template <typename Key, typename Value>
std::ostream& operator<<(std::ostream& out, const std::map<Key, Value> M) {
    out << std::string{ "{" };
    bool b = false;
    for (const auto& [key, value] : M) {
        if (b) {
            out << std::string{ ", " };
        }
        b = true;
        out << key << std::string{ ": " } << value;
    }
    out << std::string{ "}" };
    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cout << std::boolalpha;
        std::cout << file << std::string{ "(" } << line << std::string{ "): " } << func << std::string{ ": " };
        std::cout << std::string{ "ASSERT_EQUAL(" } << t_str << std::string{ ", " } << u_str << std::string{ ") failed: " };
        std::cout << t << std::string{ " != " } << u << std::string{ "." };
        if (!hint.empty()) {
            std::cout << std::string{ " Hint: " } << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

void AssertEqualImpl(const double t, const double u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint);

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, std::string{""})

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, std::string{""})

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

#define RUN_TEST(func)  func()
#define RUN_TEST_WITH_ARG(func) func