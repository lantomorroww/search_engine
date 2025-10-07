#include "gtest/gtest.h"
#include "invertedindex.h"
#include "searchserver.h"
#include "converterjson.h" // Для доступа к RelativeIndex
#include <vector>
#include <string>

using namespace std;

// Вспомогательная функция для сравнения векторов RelativeIndex
bool compareRelativeVectors(const vector<RelativeIndex>& a, const vector<RelativeIndex>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].doc_id != b[i].doc_id) return false;
        if (std::fabs(a[i].rank - b[i].rank) > 1e-6) return false;
    }
    return true;
}

// Вспомогательная функция для тестирования InvertedIndex
void TestInvertedIndexFunctionality(
    const vector<string>& docs,
    const vector<string>& requests,
    const std::vector<vector<Entry>>& expected
) {
    std::vector<std::vector<Entry>> result;
    InvertedIndex idx;
    idx.UpdateDocumentBase(docs);
    for (auto& request : requests) {
        std::vector<Entry> word_count = idx.GetWordCount(request);
        result.push_back(word_count);
    }
    ASSERT_EQ(result, expected);
}

// Тест 1: Basic functionality
TEST(TestCaseInvertedIndex, TestBasic) {
    const vector<string> docs = {
        "london is the capital of great britain",
        "big ben is the nickname for the Great bell of the striking clock"
    };
    const vector<string> requests = { "london", "the" };
    const vector<vector<Entry>> expected = {
        {
            {0, 1}
        }, {
            {0, 1}, {1, 3}
        }
    };
    TestInvertedIndexFunctionality(docs, requests, expected);
}

// Тест 2: Multiple documents and words
TEST(TestCaseInvertedIndex, TestBasic2) {
    const vector<string> docs = {
        "milk milk milk milk water water water",
        "milk water water",
        "milk milk milk milk milk water water water water water",
        "americano cappuccino"
    };
    const vector<string> requests = { "milk", "water", "cappuccino" };
    const vector<vector<Entry>> expected = {
        {
            {0, 4}, {1, 1}, {2, 5}
        }, {
            {0, 3}, {1, 2}, {2, 5}
        }, {
            {3, 1}
        }
    };
    TestInvertedIndexFunctionality(docs, requests, expected);
}

// Тест 3: Missing words
TEST(TestCaseInvertedIndex, TestInvertedIndexMissingWord) {
    const vector<string> docs = {
        "a b c d e f g h i j k l",
        "statement"
    };
    const vector<string> requests = { "m", "statement" };
    const vector<vector<Entry>> expected = {
        {
        }, {
            {1, 1}
        }
    };
    TestInvertedIndexFunctionality(docs, requests, expected);
}

// Тесты для SearchServer

// Тест 4: Simple search
TEST(TestCaseSearchServer, TestSimple) {
    const vector<string> docs = {
        "milk milk milk milk water water water",
        "milk water water",
        "milk milk milk milk milk water water water water water",
        "americano cappuccino"
    };
    const vector<string> request = { "milk water", "sugar" };
    const std::vector<vector<RelativeIndex>> expected = {
        {
            {2, 1.0f},
            {0, 0.7f},
            {1, 0.3f}
        },
        {
        }
    };
    InvertedIndex idx;
    idx.UpdateDocumentBase(docs);
    SearchServer srv(idx);
    std::vector<vector<RelativeIndex>> result = srv.search(request);

    ASSERT_EQ(result.size(), expected.size());
    for (size_t i = 0; i < result.size(); ++i) {
        EXPECT_TRUE(compareRelativeVectors(result[i], expected[i]));
    }
}

// Тест 5: Top 5 results
TEST(TestCaseSearchServer, TestTop5) {
    const vector<string> docs = {
        "london is the capital of great britain",
        "paris is the capital of france",
        "berlin is the capital of germany",
        "rome is the capital of italy",
        "madrid is the capital of spain",
        "lisboa is the capital of portugal",
        "bern is the capital of switzerland",
        "moscow is the capital of russia",
        "kiev is the capital of ukraine",
        "minsk is the capital of belarus",
        "astana is the capital of kazakhstan",
        "beijing is the capital of china",
        "tokyo is the capital of japan",
        "bangkok is the capital of thailand",
        "welcome to moscow the capital of russia the third rome",
        "amsterdam is the capital of netherlands",
        "helsinki is the capital of finland",
        "oslo is the capital of norway",
        "stockholm is the capital of sweden",
        "riga is the capital of latvia",
        "tallinn is the capital of estonia",
        "warsaw is the capital of poland",
    };
    const vector<string> request = { "moscow is the capital of russia" };
    const std::vector<vector<RelativeIndex>> expected = {
        {
            {7, 1.0f},
            {14, 1.0f},
            {0, 0.666666687f},
            {1, 0.666666687f},
            {2, 0.666666687f}
        }
    };
    InvertedIndex idx;
    idx.UpdateDocumentBase(docs);
    SearchServer srv(idx);
    std::vector<vector<RelativeIndex>> result = srv.search(request);

    ASSERT_EQ(result.size(), expected.size());
    for (size_t i = 0; i < result.size(); ++i) {
        EXPECT_TRUE(compareRelativeVectors(result[i], expected[i]));
    }
}

// Тест 6: Empty queries
TEST(TestCaseSearchServer, TestEmptyQuery) {
    const vector<string> docs = {
        "some text here",
        "another document"
    };
    const vector<string> request = { "" };

    InvertedIndex idx;
    idx.UpdateDocumentBase(docs);
    SearchServer srv(idx);
    std::vector<vector<RelativeIndex>> result = srv.search(request);

    ASSERT_EQ(result.size(), 1);
    EXPECT_TRUE(result[0].empty());
}

// Тест 7: ConverterJSON basic functionality
TEST(TestCaseConverterJSON, TestConfigLoading) {
    ConverterJSON converter;

    // Тест загрузки документов
    EXPECT_NO_THROW({
        auto docs = converter.GetTextDocuments();
        EXPECT_FALSE(docs.empty());
        });

    // Тест получения лимита ответов
    EXPECT_NO_THROW({
        int limit = converter.GetResponsesLimit();
        EXPECT_GT(limit, 0);
        });

    // Тест загрузки запросов
    EXPECT_NO_THROW({
        auto requests = converter.GetRequests();
    // Может быть пустым, но не должно бросать исключение
        });
}

// Тест 8: Single word query
TEST(TestCaseSearchServer, TestSingleWord) {
    const vector<string> docs = {
        "apple banana cherry",
        "banana orange",
        "cherry cherry cherry"
    };
    const vector<string> request = { "cherry" };

    InvertedIndex idx;
    idx.UpdateDocumentBase(docs);
    SearchServer srv(idx);
    auto result = srv.search(request);

    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].size(), 2);
    EXPECT_EQ(result[0][0].doc_id, 2); // Документ с наибольшим количеством "cherry"
    EXPECT_EQ(result[0][1].doc_id, 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}