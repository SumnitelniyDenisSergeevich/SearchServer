#include "read_input_functions.h"
#include "search_server.h"
#include "document.h"

#include <string>
#include <vector>
#include <stdexcept>

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::invalid_argument & e) {
        std::cout << std::string{ "������ ���������� ��������� " } << document_id << std::string{ ": " } << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    std::cout << std::string{ "���������� ������ �� �������: " } << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const std::invalid_argument & e) {
        std::cout << std::string{ "������ ������: " } << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    try {
        std::cout << std::string{ "������� ���������� �� �������: " } << query << std::endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const std::invalid_argument & e) {
        std::cout << std::string{ "������ �������� ���������� �� ������ " } << query << std::string{ ": " } << e.what() << std::endl;
    }
}