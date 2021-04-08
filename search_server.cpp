#include "string_processing.h"
#include "document.h"
#include "search_server.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <set>
#include <cmath>

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
                                                        // from string container
{
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument(std::string{ "id ��������� ������ ���� >= 0!" });
    }

    if (documents_.count(document_id)) {
        throw std::invalid_argument(std::string{ "�������� � ����� id ��� ����������!" });
    }

    if (CheckingForSpecialSymbols(document)) {
        throw std::invalid_argument(std::string{ "�������� �������� ����������� ������� � ����� �� 0 �� 31" });
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.push_back(document_id);
}


std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

int SearchServer::GetDocumentId(int index) const {
    return document_ids_.at(index);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::CheckingForEmptyMinusWord(const std::string& s) const {
    for (int i = 0; i < s.size(); ++i) {
        if ((s[i] == '-') && (i == (s.size() - 1))) {
            return true;
        }
        if (s[i] == '-' && s[i + 1] == ' ') {
            return true;
        }
    }
    return false;
}

bool SearchServer::CheckingForDoubleMinus(const std::string& text) const {
    for (int i = 0; i + 1 < text.size(); ++i) {
        if (text[i] == '-' && text[i + 1] == '-') {
            return true;
        }
    }
    return false;
}

bool SearchServer::CheckingForSpecialSymbols(const std::string& s) {
    for (const char c : s) {
        if (c >= 0 && c <= 31)
            return true;
    }
    return false;
}

void SearchServer::ChekingRawQuery(const std::string& raw_query) const {
    if (CheckingForSpecialSymbols(raw_query)) {
        throw std::invalid_argument(std::string{ "��������� ������ �������� ����������� ������� � ����� �� 0 �� 31" });
    }
    if (CheckingForEmptyMinusWord(raw_query)) {
        throw std::invalid_argument(std::string{ "� ��������� ������������ ������ ����� �����" });
    }
    if (CheckingForDoubleMinus(raw_query)) {
        throw std::invalid_argument(std::string{ "� ��������� ������������ ����� � 2 �������� � ������" });
    }
}


std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (CheckingForSpecialSymbols(word)) {
            throw std::invalid_argument(std::string{ "Word " } +word + std::string{ " is invalid" });
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
    if (text.empty()) {
        throw std::invalid_argument(std::string{ "Query word is empty" });
    }
    std::string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || CheckingForSpecialSymbols(word)) {
        throw std::invalid_argument(std::string{ "Query word " } +text + std::string{ " is invalid" });
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query result;
    for (const std::string& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status) {
    std::cout << std::string{ "{ " }
        << std::string{ "document_id = " } << document_id << std::string{ ", " }
        << std::string{ "status = " } << static_cast<int>(status) << std::string{ ", " }
    << std::string{ "words =" };
    for (const std::string& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << std::string{ "}" } << std::endl;
}
