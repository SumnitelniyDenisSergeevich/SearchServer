#include "document.h"

#include <string>

std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << std::string{ "{ " }
        << std::string{ "document_id = " } << document.id << std::string{ ", " }
        << std::string{ "relevance = " } << document.relevance << std::string{ ", " }
    << std::string{ "rating = " } << document.rating << std::string{ " }" };
    return out;
}

void PrintDocument(const Document& document) {
    std::cout << std::string{ "{ " }
        << std::string{ "document_id = " } << document.id << std::string{ ", " }
        << std::string{ "relevance = " } << document.relevance << std::string{ ", " }
    << std::string{ "rating = " } << document.rating << std::string{ " }" } << std::endl;
}
