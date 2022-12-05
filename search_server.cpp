#include "search_server.h"

// ===================== Public ===================== 

bool SearchServer::IsValidWord(const std::string& word) {
    return std::none_of(word.begin(), word.end(), [](char ch) { return ch >= '\0' && ch < ' '; });
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Document's ID must not be negative");
    }
    if (documents_.count(document_id) == 1) {
        throw std::invalid_argument("Document's ID must not already exist");
    }
    const std::vector<std::string> words = SearchServer::SplitIntoWordsNoStop(document);
    for (const std::string& word : words) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Document data must not contain control characters");
        }
    }
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_id_freq_[word][document_id] += inv_word_count;
        id_to_word_freq_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, SearchServer::DocumentData{ SearchServer::ComputeAverageRating(ratings), status });
    documents_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(raw_query,
        [status](int document_id, DocumentStatus document_status, int rating)
        { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    SearchServer::Query query = SearchServer::ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_id_freq_.count(word) == 0) {
            continue;
        }
        if (word_to_id_freq_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_id_freq_.count(word) == 0) {
            continue;
        }
        if (word_to_id_freq_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

const std::set<int>::const_iterator SearchServer::begin() const {
    return documents_ids_.begin();
}

const std::set<int>::const_iterator SearchServer::end() const {
    return documents_ids_.end();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string, double> EMPTY;
    if (documents_.count(document_id) == 0) {
        return EMPTY;
    }
    return id_to_word_freq_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    if (documents_.count(document_id) == 0) {
        return;
    }
    std::vector<std::string> words_to_remove;
    words_to_remove.reserve(id_to_word_freq_.at(document_id).size());

    for (const auto& [word, _] : id_to_word_freq_.at(document_id)) {
        word_to_id_freq_[word].erase(document_id);
    }
    id_to_word_freq_.erase(document_id);
    documents_ids_.erase(find(documents_ids_.begin(), documents_ids_.end(), document_id));
    documents_.erase(document_id);
}

// ===================== Private ===================== 

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!SearchServer::IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (SearchServer::IsValidWord(text) && !text.empty() && text[0] != '-') {
        return SearchServer::QueryWord{ text, is_minus, SearchServer::IsStopWord(text) };
    }
    else {
        throw std::invalid_argument("Invalid word or control character in ParseQueryWord()");
    }
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    SearchServer::Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        SearchServer::QueryWord query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            }
            else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(SearchServer::GetDocumentCount() * 1.0 / word_to_id_freq_.at(word).size());
}
