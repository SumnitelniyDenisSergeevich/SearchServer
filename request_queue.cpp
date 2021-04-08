#include "search_server.h"
#include "request_queue.h"

#include <string>
#include <vector>

RequestQueue::RequestQueue(const SearchServer& search_server) : obj(search_server) {
    // напишите реализацию
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
    return count_if(requests_.begin(), requests_.end(), [](const auto& request_) {
        return request_.is_non_empty == false;
    });
}