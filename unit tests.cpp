#include "unit tests.h"
// TO DO: improve unit test  *************************************************
// there are many standard cases which are not tested

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint) {
    if (!value) {
        std::cerr << file << "(" << line << "): " << func << ": ";
        std::cerr << "ASSERT(" << expr_str << ") failed.";
        if (!hint.empty()) {
            std::cerr << " Hint: " << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server("");
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in");
        ASSERT(found_docs.size() == 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the");
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in").empty(),
            "Stop words must be excluded from documents");
    }
}

void TestAddDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server("");
        ASSERT(server.GetDocumentCount() == 0);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.GetDocumentCount() == 1);
        auto found_docs = server.FindTopDocuments("cat");
        ASSERT_EQUAL(found_docs.at(0).id, doc_id);
    }
}

void TestExcludeDocumentsWithMinusWords() {
    const int doc_id1 = 42, doc_id2 = 11;
    const std::string content1 = "cat in the city";
    const std::string content2 = "dog in the city";
    const std::vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server("");
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL_HINT(server.FindTopDocuments("city -cat").size(), 1,
            "Documents with minus-words must be excluded from search result.");
        ASSERT_EQUAL_HINT(server.FindTopDocuments("city -dog").size(), 1,
            "Documents with minus-words must be excluded from search result.");
        ASSERT_HINT(server.FindTopDocuments("cat -city").empty(),
            "Documents with minus-words must be excluded from search result.");
    }
}

void TestDocumentMatch() {
    const int doc_id = 42;
    const std::string content = "cat dog";
    const std::vector<int> ratings = { 1, 2, 3 };
    DocumentStatus expected_status = DocumentStatus::ACTUAL;
    {
        SearchServer server("");
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        // <tie>s were replaced with another construction
        {
            auto [words, status] = server.MatchDocument("cat dog", 42);
            std::vector<std::string> expected_words = { "cat", "dog" };
            ASSERT_EQUAL_HINT(words, expected_words,
                "Words from query, which presents in the document, must be returned.");
            ASSERT(status == expected_status);
        }
        {
            auto [words, status] = server.MatchDocument("cat safari", 42);
            std::vector<std::string> expected_words = { "cat" };
            ASSERT_HINT(words == expected_words,
                "Words from query, which presents in the document, must be returned.");
            ASSERT(status == expected_status);
        }
        {
            auto [words, status] = server.MatchDocument("", 42);
            ASSERT_HINT(words.empty(), "Empty return must be for empty query.");
            ASSERT(status == expected_status);
        }
        {
            auto [words, status] = server.MatchDocument("-cat safari", 42);
            ASSERT_HINT(words.empty(), "There is no word must be returned.");
            ASSERT(status == expected_status);
        }
        {
            auto [words, status] = server.MatchDocument("-cat safari", 42);
            ASSERT_HINT(words.empty(), "There is no word must be returned.");
            ASSERT(status == expected_status);
        }
        {
            // TO DO: rewrite this test *************************************************
            SearchServer server1("safari");
            auto [words, status] = server1.MatchDocument("cat safari", 42);
            std::vector<std::string> expected_words = { "cat" };
            ASSERT_EQUAL_HINT(words, expected_words,
                "Words from query, which presents in the document, must be returned.");
            ASSERT(status == expected_status);
        }
    }
}

void TestDocumentsSortByRelevance() {
    const int doc_id1 = 11, doc_id2 = 22;
    {
        SearchServer server("");
        server.AddDocument(doc_id1, "cat in the city", DocumentStatus::ACTUAL, { 1, 1, 1 });
        server.AddDocument(doc_id2, "cat with a long tail", DocumentStatus::ACTUAL, { 2, 2, 2 });
        const auto found_docs = server.FindTopDocuments("cat city");
        ASSERT_HINT(found_docs.at(0).relevance >= found_docs.at(1).relevance,
            "Documents must be sorted in descending order.");
    }
}

void TestDocumentRating() {
    const int doc_id = 42;
    {
        SearchServer server("");
        std::vector<int> rating = { 2, 2, 2 };
        int expected_rating = accumulate(rating.begin(), rating.end(), 0)
            / static_cast<int>(rating.size());
        server.AddDocument(doc_id, "cat in the city", DocumentStatus::ACTUAL, rating);
        const int average = server.FindTopDocuments("cat").at(0).rating;
        ASSERT_HINT(average == expected_rating, "Returned rating must be equal to average.");
    }
    {
        SearchServer server("");
        std::vector<int> rating = { 1, 2, 4 };
        int expected_rating = std::accumulate(rating.begin(), rating.end(), 0)
            / static_cast<int>(rating.size());
        server.AddDocument(doc_id, "cat in the city", DocumentStatus::ACTUAL, rating);
        const int average = server.FindTopDocuments("cat").at(0).rating;
        ASSERT_HINT(average == expected_rating, "Returned rating must be equal to average.");
    }
    {
        SearchServer server("");
        std::vector<int> rating = {};
        int expected_rating = 0;
        server.AddDocument(doc_id, "cat in the city", DocumentStatus::ACTUAL, rating);
        const int average = server.FindTopDocuments("cat").at(0).rating;
        ASSERT_HINT(average == expected_rating, "If there are no ratings, zero must be returned.");
    }
}

void TestPredicateFilter() {
    const int doc_id = 42;
    const std::vector<int> ratings = { 1, 2, 3 };
    { // testing query by document id
        SearchServer server("");
        server.AddDocument(doc_id, "cat in the city", DocumentStatus::ACTUAL, ratings);
        auto found_docs = server.FindTopDocuments("cat tail",
            [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        ASSERT(found_docs.size() == 1);
        found_docs = server.FindTopDocuments("cat tail",
            [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 1; });
        ASSERT(found_docs.empty());
    }
    { // testing query by status
        SearchServer server("");
        server.AddDocument(doc_id, "cat in the city", DocumentStatus::ACTUAL, ratings);
        auto found_docs = server.FindTopDocuments("cat tail",
            [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
        ASSERT(found_docs.size() == 1);
        found_docs = server.FindTopDocuments("cat tail",
            [](int document_id, DocumentStatus status, int rating) { return status != DocumentStatus::ACTUAL; });
        ASSERT(found_docs.empty());
    }
    { // testing query by rating
        SearchServer server("");
        server.AddDocument(doc_id, "cat in the city", DocumentStatus::ACTUAL, ratings);
        auto found_docs = server.FindTopDocuments("cat tail",
            [](int document_id, DocumentStatus status, int rating) { return rating == 2; });
        ASSERT(found_docs.size() == 1);
        found_docs = server.FindTopDocuments("cat tail",
            [](int document_id, DocumentStatus status, int rating) { return rating != 2; });
        ASSERT(found_docs.empty());
    }
}

void TestSearchDocumentsWithStatus() {
    const int doc_id1 = 11, doc_id2 = 22, doc_id3 = 33, doc_id4 = 44;
    {
        SearchServer server("");
        auto found_docs = server.FindTopDocuments("cat", DocumentStatus::ACTUAL);
        ASSERT(found_docs.size() == 0);
        found_docs = server.FindTopDocuments("cat", DocumentStatus::BANNED);
        ASSERT(found_docs.size() == 0);
        found_docs = server.FindTopDocuments("cat", DocumentStatus::REMOVED);
        ASSERT(found_docs.size() == 0);
        found_docs = server.FindTopDocuments("cat", DocumentStatus::IRRELEVANT);
        ASSERT(found_docs.size() == 0);

        server.AddDocument(doc_id1, "cat in the city", DocumentStatus::ACTUAL, { 1, 2, 3 });
        found_docs = server.FindTopDocuments("cat", DocumentStatus::ACTUAL);
        ASSERT(found_docs.size() == 1);

        server.AddDocument(doc_id2, "cat in the city", DocumentStatus::BANNED, { 4, 5, 6 });
        found_docs = server.FindTopDocuments("cat", DocumentStatus::BANNED);
        ASSERT(found_docs.size() == 1);

        server.AddDocument(doc_id3, "cat in the city", DocumentStatus::REMOVED, { 2, 2, 4 });
        found_docs = server.FindTopDocuments("cat", DocumentStatus::REMOVED);
        ASSERT(found_docs.size() == 1);

        server.AddDocument(doc_id4, "cat in the city", DocumentStatus::IRRELEVANT, { 8, 8, 8 });
        found_docs = server.FindTopDocuments("cat", DocumentStatus::IRRELEVANT);
        ASSERT(found_docs.size() == 1);
    }
}

void TestRelevanceCalculation() {
    const int doc_id1 = 11, doc_id2 = 22, doc_id3 = 33;
    const std::string query = "cat";
    const size_t words_in_query = SplitIntoWords(query).size();
    const std::string content1 = "cat dog mouse";
    const std::string content2 = "cat dog";
    const std::string content3 = "cat";
    const size_t words_in_doc1 = SplitIntoWords(content1).size();
    const size_t words_in_doc2 = SplitIntoWords(content2).size();
    const size_t words_in_doc3 = SplitIntoWords(content3).size();
    {
        SearchServer server("");
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, { 1, 1, 1 });
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, { 2, 2, 2 });
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, { 3, 3, 3 });
        auto found_docs = server.FindTopDocuments(query);

        // IDF must be calculated for each word from query
        ASSERT_HINT(!found_docs.empty(), "<found_docs> in this test must not be empty.");
        double idf = log(static_cast<double>(server.GetDocumentCount()) / found_docs.size());
        // TF must be calculated for each word from query by each document
        double tf1 = static_cast<double>(words_in_query) / words_in_doc1;
        double tf2 = static_cast<double>(words_in_query) / words_in_doc2;
        double tf3 = static_cast<double>(words_in_query) / words_in_doc3;
        // resulting relevance is a sum of corresponding products of TFs and IDFs
        ASSERT(std::abs(found_docs.at(0).relevance - idf * tf3) < EPSILON);
        ASSERT(std::abs(found_docs.at(1).relevance - idf * tf2) < EPSILON);
        ASSERT(std::abs(found_docs.at(2).relevance - idf * tf1) < EPSILON);
    }
}