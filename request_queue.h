#pragma once

#include "search_server.h"

#include <string>
#include <vector>
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server) : obj_(search_server) {
    }
    template <typename DocumentPredicate>
    [[nodiscard]] std::vector<Document> AddFindRequest(const std::string_view& raw_query, DocumentPredicate document_predicate);

    [[nodiscard]] std::vector<Document> AddFindRequest(const std::string_view& raw_query, DocumentStatus status);

    [[nodiscard]] std::vector<Document> AddFindRequest(const std::string_view& raw_query);

    [[nodiscard]] int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::vector<Document> result;
        bool is_non_empty;
    };
    std::deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
    const SearchServer& obj_;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string_view& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> result = obj_.FindTopDocuments(std::execution::seq ,raw_query, document_predicate);
    if (!result.empty()) {
        if (requests_.size() >= sec_in_day_) {
            requests_.push_back(QueryResult{ result , true });
            requests_.pop_front();
        }
        else {
            requests_.push_back(QueryResult{ result , true });
        }
    }
    else {
        if (requests_.size() >= sec_in_day_) {
            requests_.push_back(QueryResult{ result , false });
            requests_.pop_front();
        }
        else {
            requests_.push_back(QueryResult{ result , false });
        }
    }
    return result;
}