#include "searchserver.h"
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <cmath>

bool SearchServer::isLatinChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

std::vector<std::vector<RelativeIndex>> SearchServer::search(const std::vector<std::string>& queries_input) {
    std::vector<std::vector<RelativeIndex>> result;

    for (const auto& query : queries_input) {
        // Разбиваем запрос на слова
        auto words = splitQuery(query);
        if (words.empty()) {
            result.push_back({});
            continue;
        }

        // Уникальные слова
        std::set<std::string> unique_words(words.begin(), words.end());

        // Кэшируем размеры списков для сортировки по редкости
        std::vector<std::pair<std::string, size_t>> word_sizes;
        for (const auto& word : unique_words) {
            auto entries = _index.GetWordCount(word);
            word_sizes.emplace_back(word, entries.size());
        }

        // Сортируем по возрастанию частоты (самые редкие первые)
        std::sort(word_sizes.begin(), word_sizes.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

        // Находим общие документы
        std::map<size_t, size_t> doc_relevance; // doc_id -> absolute relevance

        // Начинаем с самого редкого слова
        if (!word_sizes.empty()) {
            auto first_word_entries = _index.GetWordCount(word_sizes[0].first);
            for (const auto& entry : first_word_entries) {
                doc_relevance[entry.doc_id] += entry.count;
            }

            // Проверяем остальные слова
            for (size_t i = 1; i < word_sizes.size(); ++i) {
                auto word_entries = _index.GetWordCount(word_sizes[i].first);
                std::set<size_t> current_docs;
                for (const auto& entry : word_entries) {
                    current_docs.insert(entry.doc_id);
                }

                // Удаляем документы, которых нет в текущем слове
                for (auto it = doc_relevance.begin(); it != doc_relevance.end(); ) {
                    if (!current_docs.count(it->first)) {
                        it = doc_relevance.erase(it);
                    }
                    else {
                        // Находим count для этого документа в текущем слове
                        for (const auto& entry : word_entries) {
                            if (entry.doc_id == it->first) {
                                it->second += entry.count;
                                break;
                            }
                        }
                        ++it;
                    }
                }
            }
        }

        // Нормировка релевантности
        std::vector<RelativeIndex> relative_relevance;
        if (!doc_relevance.empty()) {
            // Находим максимальную абсолютную релевантность
            size_t max_abs_relevance = 0;
            for (const auto& [doc_id, relevance] : doc_relevance) {
                if (relevance > max_abs_relevance) {
                    max_abs_relevance = relevance;
                }
            }

            // Конвертируем в относительную релевантность
            for (const auto& [doc_id, abs_relevance] : doc_relevance) {
                float rank = (max_abs_relevance > 0) ?
                    static_cast<float>(abs_relevance) / max_abs_relevance : 0.0f;
                relative_relevance.push_back({ doc_id, rank });
            }

            // Сортируем по убыванию rank, при равенстве - по возрастанию doc_id
            std::sort(relative_relevance.begin(), relative_relevance.end(),
                [](const RelativeIndex& a, const RelativeIndex& b) {
                    if (std::fabs(a.rank - b.rank) < 1e-6) {
                        return a.doc_id < b.doc_id; // тай-брейк
                    }
                    return a.rank > b.rank;
                });
        }

        result.push_back(relative_relevance);
    }

    return result;
}

std::vector<std::string> SearchServer::splitQuery(const std::string& query) {
    std::vector<std::string> words;
    std::stringstream ss(query);
    std::string word;

    while (ss >> word && words.size() < 10) { // максимум 10 слов в запросе
        // Нормализация как в InvertedIndex
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        word.erase(std::remove_if(word.begin(), word.end(),
            [](char c) { return !isLatinChar(c); }), word.end());

        if (!word.empty() && word.size() <= 100) {
            words.push_back(word);
        }
    }

    return words;
}