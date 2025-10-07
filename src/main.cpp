#include "converterjson.h"
#include "invertedindex.h"
#include "searchserver.h"
#include <iostream>
#include <vector>

int main() {
    try {
        std::cout << "Starting Search Engine..." << std::endl;

        ConverterJSON converter;

        // Получаем документы с обработкой ошибок
        auto docs = converter.GetTextDocuments();
        std::cout << "Loaded " << docs.size() << " documents" << std::endl;

        if (docs.empty()) {
            std::cerr << "No documents to index!" << std::endl;
            return 1;
        }

        // Получаем запросы с проверкой ограничений
        auto requests = converter.GetRequests();
        std::cout << "Processing " << requests.size() << " requests" << std::endl;

        // Индексируем документы
        InvertedIndex index;
        index.UpdateDocumentBase(docs);

        // Выполняем поиск
        SearchServer server(index);
        auto results = server.search(requests);

        // Применяем max_responses перед записью в JSON
        int max_responses = converter.GetResponsesLimit();
        for (auto& result : results) {
            if (result.size() > max_responses) {
                result.resize(max_responses);
            }
        }

        // Сохраняем результаты
        converter.putAnswers(results);
        std::cout << "Search completed successfully!" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}