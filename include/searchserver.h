#pragma once

#include "invertedindex.h"
#include "converterjson.h"
#include <vector>
#include <string>

class SearchServer {
public:
    SearchServer(InvertedIndex& idx) : _index(idx) { }
    std::vector<std::vector<RelativeIndex>> search(const std::vector<std::string>& queries_input);

private:
    InvertedIndex& _index;
    static bool isLatinChar(char c);
    std::vector<std::string> splitQuery(const std::string& query);
};