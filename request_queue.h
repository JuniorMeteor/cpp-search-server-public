#pragma once
#include <string>
#include <vector>
#include <queue>
#include "document.h"
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
        : server_(search_server) {}

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        QueryResult result = { raw_query, server_.FindTopDocuments(raw_query, document_predicate) };
        return ProcessRequest(result);
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::string query;
        std::vector<Document> found_documents_;
    };

    int empty_requests_count_ = 0;
    const static int min_in_day_ = 1440;
    std::deque<QueryResult> requests_;
    const SearchServer& server_;
    std::vector<Document> ProcessRequest(QueryResult result);
};