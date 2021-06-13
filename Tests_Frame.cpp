#include "Tests_Frame.h"

#include <string>


void AssertEqualImpl(const double t, const double u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (abs(t - u) > 1e-6) {
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

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint) {
    if (!value) {
        std::cout << file << std::string{ "(" } << line << std::string{ "): " } << func << std::string{ ": " };
        std::cout << std::string{ "ASSERT(" } << expr_str << std::string{ ") failed." };
        if (!hint.empty()) {
            std::cout << std::string{ " Hint: " } << hint;
        }
        std::cout << std::endl;
        abort();
    }
}