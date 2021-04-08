#include "document.h"

#include <iostream>
#include <string>

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating) {
}

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
