#include "test_example_functions.h"

void AddDocument(SearchServer& search_server, const  int document_id, const std::string document_content,
    const  DocumentStatus status, const  std::vector<int>& ratings) {
    search_server.AddDocument(document_id, document_content, status, ratings);
}