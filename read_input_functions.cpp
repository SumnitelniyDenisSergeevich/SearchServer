#include "read_input_functions.h"

#include <stdexcept>

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::invalid_argument & e) {
        std::cout << std::string{ "Ошибка добавления документа " } << document_id << std::string{ ": " } << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    std::cout << std::string{ "Результаты поиска по запросу: " } << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const std::invalid_argument & e) {
        std::cout << std::string{ "Ошибка поиска: " } << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string_view& query) {
    try {
        std::cout << std::string{ "Матчинг документов по запросу: " } << query << std::endl;

        for (const int id : search_server) {
            const auto [words, status] = search_server.MatchDocument(query, id);
            PrintMatchDocumentResult(id, words, status);
        }

    }
    catch (const std::invalid_argument & e) {
        std::cout << std::string{ "Ошибка матчинга документов на запрос " } << query << std::string{ ": " } << e.what() << std::endl;
    }
}