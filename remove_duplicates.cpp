#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::map<std::set<std::string>, int> words_in_docs;
    std::vector<int> duplicates_to_remove;

    duplicates_to_remove.reserve(search_server.GetDocumentCount());
    for (auto iter_id = search_server.begin(); iter_id != search_server.end(); ++iter_id) {
        std::set<std::string> words;
        for (auto& [word, _] : search_server.GetWordFrequencies(*iter_id)) {
            words.insert(word);
        }
        
        if (words_in_docs.count(words) != 0) {
            duplicates_to_remove.push_back(*iter_id);
        } else {
            words_in_docs.insert({words, *iter_id});
        }
        
    }
    for (int duplicate : duplicates_to_remove) {
        std::cout << "Found duplicate document id " << duplicate << std::endl;
        search_server.RemoveDocument(duplicate);
    }
}