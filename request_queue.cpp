#include "request_queue.h"

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    RequestQueue::QueryResult result = { raw_query, server_.FindTopDocuments(raw_query, status) };
    return ProcessRequest(result);
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    return RequestQueue::AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
    return empty_requests_count_;
}

std::vector<Document> RequestQueue::ProcessRequest(RequestQueue::QueryResult result) {
    requests_.push_back(result);
    if (result.found_documents_.empty()) {
        ++empty_requests_count_;
    }
    if (requests_.size() > min_in_day_) {
        if (requests_.front().found_documents_.empty()) {
            --empty_requests_count_;
        }
        requests_.pop_front();
    }

    return result.found_documents_;
}