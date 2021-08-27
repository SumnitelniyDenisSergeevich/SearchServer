#pragma once

#include "string_processing.h"
#include "document.h"
#include "concurrent_map.h"

#include "log_duration.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <execution>
#include <map>
#include <set>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

using namespace std::string_literals;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        for (const std::string& word : stop_words_) {
            if (CheckingForSpecialSymbols(word)) {
                throw std::invalid_argument("���� ����� �������� ����������� ������� � ����� �� 0 �� 31"s);
            }
        }
    }

    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

    explicit SearchServer(const std::string_view& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

    template <typename DocumentPredicate, typename ExecutionPolicy>
    [[nodiscard]] std::vector<Document> FindTopDocuments(ExecutionPolicy policy, const std::string_view& raw_query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    [[nodiscard]] std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy>
    [[nodiscard]] std::vector<Document> FindTopDocuments(ExecutionPolicy policy, const std::string_view& raw_query, DocumentStatus status) const;
    [[nodiscard]] std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const;
    [[nodiscard]] std::vector<Document> FindTopDocuments(const std::string_view& raw_query) const;

    void AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings);
    
    [[nodiscard]] inline int GetDocumentCount() const noexcept;

    [[nodiscard]] std::vector<int>::const_iterator begin() const noexcept;
    [[nodiscard]] std::vector<int>::const_iterator end() const noexcept;

    [[nodiscard]] const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    template<typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy policy, int document_id);
    void RemoveDocument(int document_id);

    template<typename ExecutionPolicy>
    [[nodiscard]] std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(ExecutionPolicy policy, std::string_view raw_query, int document_id) const;
    [[nodiscard]] std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument( std::string_view raw_query, int document_id) const;

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
    std::map< int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;

    template <typename DocumentPredicate, typename ExecutionPolicy>
    [[nodiscard]] std::vector<Document> FindAllDocuments(ExecutionPolicy policy,const Query& query, DocumentPredicate document_predicate) const;

    [[nodiscard]] inline  bool IsContainWord(const std::string& word) const;
    [[nodiscard]] inline bool IsWordContainId(const std::string& word, const int doc_id) const;

    [[nodiscard]] inline bool IsStopWord(const std::string& word) const;

    [[nodiscard]] static bool CheckingForSpecialSymbols(const std::string_view& s);

    void ChekingRawQuery(const std::string_view& raw_query) const;

    [[nodiscard]] std::vector<std::string> SplitIntoWordsNoStop(const std::string_view& text) const;

    [[nodiscard]] static int ComputeAverageRating(const std::vector<int>& ratings);

    [[nodiscard]] QueryWord ParseQueryWord( std::string_view text) const;
    [[nodiscard]] Query ParseQuery( std::string_view text) const;

    [[nodiscard]] double ComputeWordInverseDocumentFreq(const std::string& word) const;
};

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, const std::string_view& raw_query, DocumentPredicate document_predicate) const {
    ChekingRawQuery(raw_query);
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);
    sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
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

template <typename ExecutionPolicy>
[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, const std::string_view& raw_query, DocumentStatus status) const {
        return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

template <typename DocumentPredicate>
[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template<typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy policy, int document_id) {
    if (auto iter = document_to_word_freqs_.find(document_id); iter != document_to_word_freqs_.end()) {

        std::for_each(policy, iter->second.begin(), iter->second.end(),
            [this, document_id](std::pair<const std::string_view, double>& word_freqs) {
            std::string word = static_cast<std::string>(word_freqs.first);
            word_to_document_freqs_.at(word).erase(document_id);
            if (word_to_document_freqs_.at(word).empty()) {
                word_to_document_freqs_.erase(word);
            }
        }
        );
        document_to_word_freqs_.erase(iter);
        documents_.erase(document_id);
        document_ids_.erase(std::find(document_ids_.begin(), document_ids_.end(), document_id));
    }
}

template<typename ExecutionPolicy>
[[nodiscard]] std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(ExecutionPolicy policy, std::string_view raw_query, int document_id) const {
    if (!std::count(document_ids_.begin(), document_ids_.end(), document_id)) {
        throw std::out_of_range( "invalid document id "s);
    }
    ChekingRawQuery(raw_query);
    const auto query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;

    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(),
        [this, document_id, &matched_words](const std::string& word) {
        if (word_to_document_freqs_.count(word)) {
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word_to_document_freqs_.find(word)->first);
            }
        }
    });
    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(),
        [this, document_id, &matched_words](const std::string& word) {
        if (word_to_document_freqs_.count(word)) {
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
            }
        }
    });

    return { matched_words, documents_.at(document_id).status };
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy policy, const Query& query, DocumentPredicate document_predicate) const {

    ConcurrentMap<int, double> document_to_relevance_con(4);
    {
        std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [this, document_predicate, &document_to_relevance_con](const std::string& word) {
            if (word_to_document_freqs_.count(word) == 0) {
            }
            else {
                std::for_each( word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(), [this, document_predicate, &document_to_relevance_con, &word](const std::pair<int, double>& val) {
                    const auto& document_data = documents_.at(val.first);
                    if (document_predicate(val.first, document_data.status, document_data.rating)) {
                        document_to_relevance_con[val.first].ref_to_value += val.second * ComputeWordInverseDocumentFreq(word);
                    }
                });
            }
        });
    }
    std::map<int, double> document_to_relevance = std::move(document_to_relevance_con.BuildOrdinaryMap());

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
        matched_documents.push_back({ document_id, std::move(relevance), documents_.at(document_id).rating });
    }
    return matched_documents;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words, DocumentStatus status);