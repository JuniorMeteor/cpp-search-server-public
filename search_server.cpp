#include "search_server.h"

// ===================== Public ===================== 

bool SearchServer::IsValidWord(const std::string_view& word) {
    return std::none_of(word.begin(), word.end(), [](char ch) { return ch >= '\0' && ch < ' '; });
}

void SearchServer::AddDocument(int document_id, const std::string_view& document,
    DocumentStatus status, const std::vector<int>& ratings)
{
    if (document_id < 0) {
        throw std::invalid_argument("Document's ID must not be negative");
    }
    if (documents_.count(document_id)) {
        throw std::invalid_argument("Document's ID must not already exist");
    }

    documents_.emplace(document_id,
        SearchServer::DocumentData{ SearchServer::ComputeAverageRating(ratings), status, std::string(document)});

    const std::vector<std::string_view> words = SearchServer::SplitIntoWordsNoStop(documents_.at(document_id).text);
    for (const std::string_view& word : words) {
        if (!IsValidWord(word)) {
            documents_.erase(document_id);
            throw std::invalid_argument("Document data must not contain control characters");
        }
    }

    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view& word : words) {
        word_to_id_freq_[word][document_id] += inv_word_count;
        id_to_word_freq_[document_id][word] += inv_word_count;
    }

    documents_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(raw_query,
        [status](int document_id, DocumentStatus document_status, int rating)
        { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query, int document_id) const {
    SearchServer::Query query = SearchServer::ParseQuery(std::execution::seq, raw_query);
    std::vector<std::string_view> matched_words;
    for (const std::string_view& word : query.minus_words) {
        if (word_to_id_freq_.count(word) == 0) {
            continue;
        }
        if (word_to_id_freq_.at(word).count(document_id)) {
            matched_words.clear();
            return { {}, documents_.at(document_id).status };
        }
    }
    for (const std::string_view& word : query.plus_words) {
        if (word_to_id_freq_.count(word) == 0) {
            continue;
        }
        if (word_to_id_freq_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy,
    const std::string_view& raw_query, int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy,
    const std::string_view& raw_query, int document_id) const
{
    if (!documents_ids_.count(document_id)) {
        throw std::out_of_range("Document ID does not exist.");
    }

    SearchServer::Query query = SearchServer::ParseQuery(std::execution::par, raw_query);
    std::vector<std::string_view> matched_words(query.plus_words.size());
    const std::map<std::string_view, double>& document_words = id_to_word_freq_.at(document_id);

    if (!std::none_of(std::execution::par,
        query.minus_words.begin(),
        query.minus_words.end(),
        [&document_words](const std::string_view& minus_word) {
            return document_words.count(minus_word);
        })
        )
    {
        return { {}, documents_.at(document_id).status };
    }
        std::copy_if(std::execution::par,
            query.plus_words.begin(),
            query.plus_words.end(),
            matched_words.begin(),
            [&document_words](const std::string_view& plus_word) {
                return document_words.count(plus_word);
            });

        std::sort(std::execution::par,
            matched_words.begin(),
            matched_words.end());
        matched_words.erase(
            std::unique(std::execution::par,
                matched_words.begin(), matched_words.end(),
                [](const std::string_view& lhs, const std::string_view& rhs) {
                    return lhs == rhs || rhs.empty();
                }
                ),
            matched_words.end());
        if (!matched_words.empty() && matched_words.front().empty()) {
            matched_words.erase(matched_words.begin());
        }
        return { matched_words, documents_.at(document_id).status };
}

const std::set<int>::const_iterator SearchServer::begin() const {
    return documents_ids_.begin();
}

const std::set<int>::const_iterator SearchServer::end() const {
    return documents_ids_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double> EMPTY;
    if (documents_.count(document_id) == 0) {
        return EMPTY;
    }
    return id_to_word_freq_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    SearchServer::RemoveDocument(std::execution::seq, document_id);
}
// ===================== Private ===================== 

bool SearchServer::IsStopWord(const std::string_view& word) const {
    return stop_words_.count(std::string(word)) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
    std::vector<std::string_view> words;
    for (const std::string_view& word : SplitIntoWordsView(text)) {
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
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

SearchServer::Query SearchServer::ParseQuery(std::execution::parallel_policy, const std::string_view& text) const {
    SearchServer::Query query;
    if (text.empty()) {
        return query;
    }
    for (const std::string_view& word : SplitIntoWordsView(text)) {
        SearchServer::QueryWord query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop && !query_word.data.empty()) {
            query_word.is_minus
            ? query.minus_words.push_back(query_word.data)
            : query.plus_words.push_back(query_word.data);
        }
    }
    return query;
}

SearchServer::Query SearchServer::ParseQuery(std::execution::sequenced_policy, const std::string_view& text) const {
    SearchServer::Query query;
    if (text.empty()) {
        return query;
    }
    for (const std::string_view& word : SplitIntoWordsView(text)) {
        SearchServer::QueryWord query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop && !query_word.data.empty()) {
            query_word.is_minus
                ? query.minus_words.push_back(query_word.data)
                : query.plus_words.push_back(query_word.data);
        }
    }
    std::sort(query.plus_words.begin(), query.plus_words.end());
    std::sort(query.minus_words.begin(), query.minus_words.end());
    query.plus_words.erase(
        std::unique(query.plus_words.begin(), query.plus_words.end()),
        query.plus_words.end());
    query.minus_words.erase(
        std::unique(query.minus_words.begin(), query.minus_words.end()),
        query.minus_words.end());
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const {
    return log(SearchServer::GetDocumentCount() * 1.0 / word_to_id_freq_.at(word).size());
}
