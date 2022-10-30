#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>

#include "search_server.h"

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    bool is_first = true;
    os << "[";
    for (const auto& element : vec) {
        if (is_first) {
            is_first = false;
            os << element;
        }
        else {
            os << ", " << element;
        }
    }
    os << "]";
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& s) {
    bool is_first = true;
    os << "{";
    for (const auto& element : s) {
        if (is_first) {
            is_first = false;
            os << element;
        }
        else {
            os << ", " << element;
        }
    }
    os << "}";
    return os;
}

template <typename Tkey, typename Tvalue>
std::ostream& operator<<(std::ostream& os, const std::map<Tkey, Tvalue>& m) {
    bool is_first = true;
    os << "{";
    for (const auto& [key, value] : m) {
        if (is_first) {
            is_first = false;
            os << key << ": " << value;
        }
        else {
            os << ", " << key << ": " << value;
        }
    }
    os << "}";
    return os;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "(" << line << "): " << func << ": ";
        cerr << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
        cerr << t << " != " << u << ".";
        if (!hint.empty()) {
            cerr << " Hint: " << hint;
        }
        cerr << endl;
        abort();
    }
}
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, "")
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file,
                const std::string& func, unsigned line, const std::string& hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, "")
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename F>
void RunTestImpl(F func, const std::string& func_name) {
    func();
    cerr << func_name << " OK"s << std::endl;
}
#define RUN_TEST(func)  RunTestImpl(func, #func)

// -------- UNIT TESTS BEGIN ----------

void TestExcludeStopWordsFromAddedDocumentContent();
void TestAddDocumentContent();
void TestExcludeDocumentsWithMinusWords();
void TestDocumentMatch();
void TestDocumentsSortByRelevance();
void TestDocumentRating();
void TestPredicateFilter();
void TestSearchDocumentsWithStatus();
void TestRelevanceCalculation();

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocumentContent);
    RUN_TEST(TestExcludeDocumentsWithMinusWords);
    RUN_TEST(TestDocumentMatch);
    RUN_TEST(TestDocumentsSortByRelevance);
    RUN_TEST(TestDocumentRating);
    RUN_TEST(TestPredicateFilter);
    RUN_TEST(TestSearchDocumentsWithStatus);
    RUN_TEST(TestRelevanceCalculation);
    // amount of tests: 9
}