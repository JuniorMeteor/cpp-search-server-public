#pragma once
#include <string>
#include <vector>
#include "search_server.h"

void AddDocument(SearchServer& search_server, const  int document_id, const std::string document_content,
                 const  DocumentStatus status, const  std::vector<int>& ratings);