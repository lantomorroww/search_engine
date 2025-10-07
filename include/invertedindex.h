#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>

struct Entry {
    size_t doc_id, count;

    bool operator ==(const Entry& other) const {
        return (doc_id == other.doc_id && count == other.count);
    }
};

class InvertedIndex {
public:
    InvertedIndex() = default;
    InvertedIndex(const InvertedIndex&) = delete;
    InvertedIndex& operator=(const InvertedIndex&) = delete;
    InvertedIndex(InvertedIndex&&) = default;
    InvertedIndex& operator=(InvertedIndex&&) = default;

    void UpdateDocumentBase(std::vector<std::string> input_docs);
    std::vector<Entry> GetWordCount(const std::string& word);

private:
    std::vector<std::string> docs;
    std::map<std::string, std::vector<Entry>> freq_dictionary;
    std::mutex dictionary_mutex;

    static bool isLatinChar(char c);
    void indexDocument(size_t doc_id, const std::string& text);
};