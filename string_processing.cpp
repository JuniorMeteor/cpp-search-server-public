#include "string_processing.h"

std::vector<std::string> SplitIntoWordsView(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

std::vector<std::string_view> SplitIntoWordsView(const std::string_view& text) {
    std::vector<std::string_view> words;
    size_t pos(0);
    size_t pos_end(text.npos);
    while (true) {
        size_t space = text.find(' ', pos);
        words.push_back(space == pos_end ? text.substr(pos) : text.substr(pos, space - pos));
        if (space == pos_end) {
            break;
        }
        else {
            pos = space + 1;
        }
    }

    return words;
}