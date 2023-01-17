#pragma once
#include <algorithm>
#include <cmath>
#include <execution>
#include <future>
#include <iterator>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>
#include "concurent_map.h"
#include "document.h"
#include "log_duration.h"
#include "read_input_functions.h"
#include "string_processing.h"

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
    
    explicit SearchServer(const std::string_view stop_words_text)
        : SearchServer(SplitIntoWords(std::string(stop_words_text))) {
    }

    explicit SearchServer(const std::string stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {
    }
    
    void AddDocument(int document_id, const std::string_view& document, DocumentStatus status,
        const std::vector<int>& ratings);
    static bool IsValidWord(const std::string_view& word);
    int GetDocumentCount() const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query, DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query, DocumentStatus status) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query,
        int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy,
        const std::string_view& raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy,
        const std::string_view& raw_query, int document_id) const;

    const std::set<int>::const_iterator begin() const;
    const std::set<int>::const_iterator end() const;
    
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    void RemoveDocument(int document_id);
    
    template <typename ExecutionPolicy>
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
    
    const std::set<std::string, std::less<>> stop_words_;

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

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy& policy, const Query& query,
        DocumentPredicate document_predicate) const;
};

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    const ExecutionPolicy& policy,
    const std::string_view& raw_query,
    DocumentPredicate document_predicate) const 
{
    Query query = ParseQuery(policy, raw_query);
    std::vector<Document> matched_documents = FindAllDocuments(policy, query, document_predicate);
    std::sort(policy, matched_documents.begin(), matched_documents.end(),
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

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(policy, raw_query,
        [status](int document_id, DocumentStatus document_status, int rating)
        { return document_status == status; });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query) const {
    return SearchServer::FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy& policy, const Query& query, DocumentPredicate document_predicate) const {
    const size_t BUCKET_COUNT = 8;
    ConcurrentMap<int, double> document_to_relevance(BUCKET_COUNT);

    const auto plus_words_func =
        [&](const std::string_view word) {
        if (word_to_id_freq_.count(word) != 0) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_id_freq_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        }
    };
    ForEach(policy, query.plus_words, plus_words_func);

    auto doc_to_rel = document_to_relevance.BuildOrdinaryMap();
    const auto minus_words_func =
        [&](const std::string_view word) {
            if (word_to_id_freq_.count(word) != 0) {
                for (const auto [document_id, _] : word_to_id_freq_.at(word)) {
                    doc_to_rel.erase(document_id);
                }
            }
    };
    ForEach(policy, query.minus_words, minus_words_func);

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : doc_to_rel) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}


//=================== ADDITIONAL FUNCTIONS ===================

template <typename ExecutionPolicy, typename ForwardRange, typename Function>
void ForEach(const ExecutionPolicy& policy, ForwardRange& range, Function function) {
    if constexpr (
        std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>
        || std::is_same_v<typename std::iterator_traits<typename ForwardRange::iterator>::iterator_category,
        std::random_access_iterator_tag>
        ) {
            std::for_each(policy, range.begin(), range.end(), function);
        }
    else {
        static constexpr int PART_COUNT = 4;
        const auto part_length = size(range) / PART_COUNT;
        auto part_begin = range.begin();
        auto part_end = next(part_begin, part_length);

        std::vector<std::future<void>> futures;
        for (int i = 0;
            i < PART_COUNT;
            ++i,
            part_begin = part_end,
            part_end = (i == PART_COUNT - 1
                ? range.end()
                : std::next(part_begin, part_length))
            ) {
            futures.push_back(async([function, part_begin, part_end] {
                std::for_each(part_begin, part_end, function);
                }));
        }
    }
}