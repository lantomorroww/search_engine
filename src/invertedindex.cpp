#include "invertedindex.h"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cctype>

bool InvertedIndex::isLatinChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void InvertedIndex::UpdateDocumentBase(std::vector<std::string> input_docs) {
    if (input_docs.empty()) {
        std::cerr << "Warning: No documents provided for indexing" << std::endl;
        return;
    }

    docs = std::move(input_docs);
    freq_dictionary.clear();

    std::vector<std::thread> threads;
    for (size_t i = 0; i < docs.size(); ++i) {
        threads.emplace_back(&InvertedIndex::indexDocument, this, i, docs[i]);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Сортируем все векторы Entry по doc_id для детерминизма
    for (auto& [word, entries] : freq_dictionary) {
        std::sort(entries.begin(), entries.end(),
            [](const Entry& a, const Entry& b) { return a.doc_id < b.doc_id; });
    }

    std::cout << "Indexed " << docs.size() << " documents. Dictionary size: "
        << freq_dictionary.size() << " words" << std::endl;
}

void InvertedIndex::indexDocument(size_t doc_id, const std::string& text) {
    if (text.empty()) {
        std::cerr << "Document " << doc_id << " is empty, skipping" << std::endl;
        return;
    }

    std::stringstream ss(text);
    std::string word;
    std::map<std::string, size_t> word_count;
    size_t total_words = 0;

    while (ss >> word && total_words < 1000) {
        // Нормализация: только нижний регистр и латинские буквы
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);

        // Удаляем не-латинские символы
        word.erase(std::remove_if(word.begin(), word.end(),
            [](char c) { return !isLatinChar(c); }), word.end());

        // Проверяем длину слова
        if (word.size() > 100) {
            word = word.substr(0, 100);
            std::cerr << "Word in document " << doc_id << " exceeded 100 characters, truncated" << std::endl;
        }

        if (!word.empty()) {
            word_count[word]++;
            total_words++;
        }
    }

    // Логирование при обрезке документа
    if (total_words >= 1000) {
        std::cerr << "Document " << doc_id << " truncated to 1000 words" << std::endl;
    }

    // Потокобезопасное обновление частотного словаря
    std::lock_guard<std::mutex> lock(dictionary_mutex);
    for (const auto& [word, count] : word_count) {
        freq_dictionary[word].push_back({ doc_id, count });
    }
}

std::vector<Entry> InvertedIndex::GetWordCount(const std::string& word) {
    if (word.empty()) {
        return {};
    }

    std::string normalized_word = word;
    std::transform(normalized_word.begin(), normalized_word.end(),
        normalized_word.begin(), ::tolower);

    auto it = freq_dictionary.find(normalized_word);
    if (it != freq_dictionary.end()) {
        return it->second; // Уже отсортировано по doc_id
    }
    return {};
}