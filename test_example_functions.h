#pragma once

#include "search_server.h"

#include <vector>
#include <string>

void AddDocument(SearchServer& search_server, const  int document_id, const std::string document_content,
    const  DocumentStatus status, const  std::vector<int>& ratings);