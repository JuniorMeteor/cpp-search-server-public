#pragma once
#include <list>
#include "document.h"
#include "search_server.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);
/*
std::vector<std::vector<Document>> ProcessQueriesSlow(const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(queries.begin(), queries.end(), result.begin(),
        [&search_server](const std::string& query) { return search_server.FindTopDocuments(query); });

    return result;
}
*/

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);