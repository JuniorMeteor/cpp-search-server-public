#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <tuple>
#include <set>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <execution>

#include "document.h"
#include "string_processing.h"
#include "log_duration.h"
#include "read_input_functions.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
             : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (const std::string& word : stop_words) {
            if (!IsValidWord(word)) {
                throw std::invalid_argument("Stop words must not contain control characters");
            }
        }
    }

    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWordsView(stop_words_text)) {
    }
    
    void AddDocument(int document_id, const std::string_view& document, DocumentStatus status,
        const std::vector<int>& ratings);
    static bool IsValidWord(const std::string_view& word);
    int GetDocumentCount() const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query,
        int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy,
        const std::string_view& raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy,
        const std::string_view& raw_query, int document_id) const
    {
        return MatchDocument(raw_query, document_id);
    }

    const std::set<int>::const_iterator begin() const;
    const std::set<int>::const_iterator end() const;
    
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    void RemoveDocument(int document_id);

    // no check for ExecutionPolicy input class!!!
    template <class ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& exe_policy, int document_id) {
        if (documents_.count(document_id) == 0) {
            return;
        }
        std::vector<const std::string_view*> ptr_words_to_remove;
        ptr_words_to_remove.reserve(id_to_word_freq_.at(document_id).size());

        std::transform(
            exe_policy,
            id_to_word_freq_[document_id].begin(),
            id_to_word_freq_[document_id].end(),
            ptr_words_to_remove.begin(),
            [](std::pair<const std::string_view, double>& element) {
                return &element.first;
            }
        );

        std::for_each(
            exe_policy,
            ptr_words_to_remove.begin(),
            ptr_words_to_remove.end(),
            [this, document_id](const std::string_view* word) {
                word_to_id_freq_[*word].erase(document_id);
                return;
            }
        );

        id_to_word_freq_.erase(document_id);
        documents_ids_.erase(std::find(exe_policy, documents_ids_.begin(), documents_ids_.end(), document_id));
        documents_.erase(document_id);
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::string text;
    };
    
    const std::set<std::string> stop_words_;

    std::map<std::string_view, std::map<int, double>> word_to_id_freq_;
    std::map<int, std::map<std::string_view, double>> id_to_word_freq_;

    std::set<int> documents_ids_;
    std::map<int, DocumentData> documents_;

private:
    bool IsStopWord(const std::string_view& word) const;
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus = false;
        bool is_stop = false;
    };
    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    Query ParseQuery(std::execution::parallel_policy,  const std::string_view& text) const;
    Query ParseQuery(std::execution::sequenced_policy, const std::string_view& text) const;

    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
};

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string & raw_query, DocumentPredicate document_predicate) const {
    Query query = ParseQuery(std::execution::seq, raw_query);
    std::vector<Document> matched_documents = FindAllDocuments(query, document_predicate);
    std::sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view& word : query.plus_words) {
        if (word_to_id_freq_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_id_freq_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view& word : query.minus_words) {
        if (word_to_id_freq_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_id_freq_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}