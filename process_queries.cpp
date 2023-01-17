#include <algorithm>
#include <execution>
#include <list>
#include <string>
#include <vector>
#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
        [&search_server](const std::string& query) { return search_server.FindTopDocuments(query); });

    return result;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    auto temp = ProcessQueries(search_server, queries);
    std::list<Document> result;
    for (auto& vec : temp) {
        result.insert(result.end(), vec.begin(), vec.end());
    }
    return result;
}