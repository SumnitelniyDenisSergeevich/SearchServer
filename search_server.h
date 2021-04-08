#pragma once

#include "string_processing.h"
#include "document.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <execution>
#include <map>
#include <set>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        for (const std::string& word : stop_words) {
            if (CheckingForSpecialSymbols(word)) {
                throw std::invalid_argument(std::string{ "стоп слова содержат специальные символы с кодом от 0 до 31" });
            }
        }
    }

    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

    template <typename DocumentPredicate>
    [[nodiscard]] std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    [[nodiscard]] std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

    [[nodiscard]] std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    [[nodiscard]] inline int GetDocumentCount() const noexcept{
        return documents_.size();
    }

    [[nodiscard]] inline int GetDocumentId(int index) const {
        return document_ids_.at(index);
    }

    [[nodiscard]] std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };


    const std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;

    template <typename DocumentPredicate>
    [[nodiscard]] std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

    [[nodiscard]] inline  bool IsContainWord(const std::string& word) const {
        return word_to_document_freqs_.count(word);
    }

    [[nodiscard]] inline bool IsWordContainId(const std::string& word, const int doc_id) const {
        return word_to_document_freqs_.at(word).count(doc_id);
    }

    [[nodiscard]] inline bool IsStopWord(const std::string& word) const {
        return stop_words_.count(word) > 0;
    }

    [[nodiscard]] bool CheckingForEmptyMinusWord(const std::string& s) const;

    [[nodiscard]] bool CheckingForDoubleMinus(const std::string& text) const;

    [[nodiscard]] static bool CheckingForSpecialSymbols(const std::string& s);

    void ChekingRawQuery(const std::string& raw_query) const;

    [[nodiscard]] std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    [[nodiscard]] static int ComputeAverageRating(const std::vector<int>& ratings);

    [[nodiscard]] QueryWord ParseQueryWord(const std::string& text) const;

    [[nodiscard]] Query ParseQuery(const std::string& text) const;

    [[nodiscard]] double ComputeWordInverseDocumentFreq(const std::string& word) const;
};

template <typename DocumentPredicate>
 std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
    ChekingRawQuery(raw_query);

    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(std::execution::par, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

 template <typename DocumentPredicate>
 std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
     std::map<int, double> document_to_relevance;
     for (const std::string& word : query.plus_words) {
         if (word_to_document_freqs_.count(word) == 0) {
             continue;
         }
         const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
         for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
             const auto& document_data = documents_.at(document_id);
             if (document_predicate(document_id, document_data.status, document_data.rating)) {
                 document_to_relevance[document_id] += term_freq * inverse_document_freq;
             }
         }
     }

     for (const std::string& word : query.minus_words) {
         if (word_to_document_freqs_.count(word) == 0) {
             continue;
         }
         for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
             document_to_relevance.erase(document_id);
         }
     }

     std::vector<Document> matched_documents;
     for (const auto [document_id, relevance] : document_to_relevance) {
         matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
     }
     return matched_documents;
 }

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status);