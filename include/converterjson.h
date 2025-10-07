#pragma once

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include "nlohmann/json.hpp"

struct RelativeIndex {
    size_t doc_id;
    float rank;

    bool operator ==(const RelativeIndex& other) const {
        return (doc_id == other.doc_id && std::fabs(rank - other.rank) < 1e-6);
    }
};

class ConverterJSON {
public:
    ConverterJSON() = default;

    /**
     * Метод получения содержимого файлов
     * @return Возвращает список с содержимым файлов перечисленных
     * в config.json
     */
    std::vector<std::string> GetTextDocuments();

    /**
     * Метод считывает поле max_responses для определения предельного
     * количества ответов на один запрос
     * @return
     */
    int GetResponsesLimit();

    /**
     * Метод получения запросов из файла requests.json
     * @return возвращает список запросов из файла requests.json
     */
    std::vector<std::string> GetRequests();

    /**
     * Положить в файл answers.json результаты поисковых запросов
     */
    void putAnswers(std::vector<std::vector<RelativeIndex>> answers);

private:
    // Вспомогательная функция для проверки латинских символов
    bool isLatinChar(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
};