#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    // set of words which corresponds to some document
    std::map<std::set<std::string>, int> words_in_docs;
    // list of duplicates
    std::vector<int> duplicates_to_remove;
    // optimization
    duplicates_to_remove.reserve(search_server.GetDocumentCount());
    // checking each doc
    for (auto iter_id = search_server.begin(); iter_id != search_server.end(); ++iter_id) {
        std::set<std::string> words;
        // get words from the doc and put them to <words>
        for (auto& [word, _] : search_server.GetWordFrequencies(*iter_id)) {
            words.insert(word);
        }
        
        if (words_in_docs.count(words)) {
            // if current set of word already exists, doc id is added to the list of duplicates
            // and it is not added to <words_in_docs>
            duplicates_to_remove.push_back(*iter_id);
        } else {
            // if not, add new <words> and first <id> which contains <words>
            words_in_docs.insert({words, *iter_id});
        }
        
    }
    for (auto& duplicate : duplicates_to_remove) {
        // removing duplicates
        std::cout << "Found duplicate document id " << duplicate << std::endl;
        search_server.RemoveDocument(duplicate);
    }
}