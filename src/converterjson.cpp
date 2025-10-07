#include "converterjson.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

std::vector<std::string> ConverterJSON::GetTextDocuments() {
    std::ifstream config_file("config.json");
    if (!config_file.is_open()) {
        throw std::runtime_error("config file is missing");
    }

    nlohmann::json config;
    try {
        config_file >> config;
    }
    catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("config.json parse error: " + std::string(e.what()));
    }

    if (config.empty()) {
        throw std::runtime_error("config file is empty");
    }

    if (!config.contains("config")) {
        throw std::runtime_error("config file missing 'config' section");
    }

    if (!config["config"].contains("version")) {
        throw std::runtime_error("config file missing 'version' field");
    }

    if (!config.contains("files")) {
        throw std::runtime_error("config file missing 'files' field");
    }

    // Проверка версии
    if (config["config"]["version"] != "0.1") {
        throw std::runtime_error("config file has incorrect file version");
    }

    std::vector<std::string> documents;
    for (const auto& file_path : config["files"]) {
        std::string path = file_path;
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "File " << path << " not found! Skipping..." << std::endl;
            continue;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        file.close();

        // Логирование при превышении ограничений
        std::stringstream ss(content);
        std::string word;
        size_t word_count = 0;
        while (ss >> word) {
            if (++word_count > 1000) {
                std::cerr << "Document " << path << " has more than 1000 words. Processing first 1000 words." << std::endl;
                break;
            }
        }

        documents.push_back(content);
    }

    if (documents.empty()) {
        std::cerr << "Warning: No documents were successfully loaded!" << std::endl;
    }

    return documents;
}

int ConverterJSON::GetResponsesLimit() {
    std::ifstream config_file("config.json");
    if (!config_file.is_open()) {
        throw std::runtime_error("config file is missing");
    }

    nlohmann::json config;
    try {
        config_file >> config;
    }
    catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("config.json parse error: " + std::string(e.what()));
    }

    if (!config.contains("config")) {
        throw std::runtime_error("config file missing 'config' section");
    }

    if (config["config"].contains("max_responses")) {
        return config["config"]["max_responses"];
    }
    return 5; // значение по умолчанию
}

std::vector<std::string> ConverterJSON::GetRequests() {
    std::ifstream requests_file("requests.json");
    if (!requests_file.is_open()) {
        throw std::runtime_error("requests.json file is missing");
    }

    nlohmann::json requests_json;
    try {
        requests_file >> requests_json;
    }
    catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("requests.json parse error: " + std::string(e.what()));
    }

    if (requests_json.empty()) {
        throw std::runtime_error("requests.json file is empty");
    }

    if (!requests_json.contains("requests")) {
        throw std::runtime_error("requests.json missing 'requests' field");
    }

    std::vector<std::string> requests_list;
    for (const auto& request : requests_json["requests"]) {
        // Проверяем ограничение на количество запросов
        if (requests_list.size() >= 1000) {
            std::cerr << "Too many requests in requests.json, limiting to 1000" << std::endl;
            break;
        }

        std::string request_str = request;

        // Проверяем количество слов в запросе
        std::stringstream ss(request_str);
        std::string word;
        size_t word_count = 0;
        while (ss >> word && word_count < 10) {
            word_count++;
        }

        if (word_count == 0) {
            std::cerr << "Empty request found, skipping" << std::endl;
            continue;
        }

        if (word_count > 10) {
            std::cerr << "Request has more than 10 words, truncating: " << request_str << std::endl;
            // Обрезаем запрос до 10 слов
            std::stringstream ss_trim(request_str);
            std::string trimmed_request;
            for (int i = 0; i < 10; ++i) {
                std::string word;
                if (ss_trim >> word) {
                    if (!trimmed_request.empty()) trimmed_request += " ";
                    trimmed_request += word;
                }
            }
            requests_list.push_back(trimmed_request);
        }
        else {
            requests_list.push_back(request_str);
        }
    }

    if (requests_list.empty()) {
        std::cerr << "Warning: No valid requests found after processing!" << std::endl;
    }

    return requests_list;
}

void ConverterJSON::putAnswers(std::vector<std::vector<RelativeIndex>> answers) {
    nlohmann::json result;
    result["answers"] = nlohmann::json::object();

    // Очищаем/пересоздаем файл
    std::ofstream file("answers.json", std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create answers.json");
    }

    for (size_t i = 0; i < answers.size(); ++i) {
        std::string request_id = "request" +
            std::string(3 - std::to_string(i + 1).length(), '0') + std::to_string(i + 1);

        if (answers[i].empty()) {
            // Только result: false (без других полей)
            result["answers"][request_id] = { {"result", "false"} };
        }
        else {
            // Формируем массив relevance
            nlohmann::json relevance_array = nlohmann::json::array();
            for (const auto& rel_index : answers[i]) {
                relevance_array.push_back({
                    {"docid", rel_index.doc_id},
                    {"rank", rel_index.rank}
                    });
            }

            // result: true + массив relevance
            result["answers"][request_id] = {
                {"result", "true"},
                {"relevance", relevance_array}
            };
        }
    }

    file << result.dump(4);
    file.close();

    std::cout << "Results written to answers.json" << std::endl;
}