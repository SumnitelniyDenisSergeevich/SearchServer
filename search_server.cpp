#include "search_server.h"

#include <cmath>



void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument(std::string{ "id документа должен быть >= 0!" });
    }

    if (documents_.count(document_id)) {
        throw std::invalid_argument(std::string{ "документ с таким id уже существует!" });
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word_to_document_freqs_.find(word)->first] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.push_back(document_id);
}


std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, status);
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if (document_to_word_freqs_.find(document_id) != document_to_word_freqs_.end()) {
        return document_to_word_freqs_.at(document_id);
    }
    static std::map<std::string_view, double> empty;
    return  empty;
}

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument( std::string_view raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

bool SearchServer::CheckingForSpecialSymbols(const std::string_view& s) {
    for (const char c : s) {
        if (c >= 0 && c <= 31)
            return true;
    }
    return false;
}

void SearchServer::ChekingRawQuery(const std::string_view& raw_query) const {
    for (size_t i = 0; i + 1 < raw_query.size(); ++i) {
        if (raw_query.at(i) == '-' && (raw_query.at(i + 1) == '-' || raw_query.at(i + 1) == ' ')) {
            throw std::invalid_argument(std::string{ "invalid query" });
        }
    }
    if (raw_query.at(raw_query.size() - 1) == '-') {
        throw std::invalid_argument(std::string{ "invalid query empty minus word" });
    }
    if (CheckingForSpecialSymbols(raw_query)) {
        throw std::invalid_argument(std::string{ "invalid query special symbols" });
    }

}


std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
    std::vector<std::string> words;
    for (const std::string_view& word : SplitIntoWords(text)) {
        if (CheckingForSpecialSymbols(word)) {
            throw std::invalid_argument(std::string{ "Word  is invalid" });
        }
        if (std::string s_word = static_cast<std::string>(word); !IsStopWord(s_word)) {

            words.push_back(s_word);
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

SearchServer::QueryWord SearchServer::ParseQueryWord( std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument(std::string{ "Query word is empty" });
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text.remove_prefix(1);
    }
    if (text.empty() || text[0] == '-' || CheckingForSpecialSymbols(text)) {
        throw std::invalid_argument(std::string{ "Query word is invalid" });
    }

    return { static_cast<std::string>(text), is_minus, IsStopWord(static_cast<std::string>(text)) };
}

SearchServer::Query SearchServer::ParseQuery( std::string_view text) const {
    Query result;
    for (const std::string_view& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            query_word.is_minus ? result.minus_words.insert(query_word.data) : result.plus_words.insert(query_word.data);
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words, DocumentStatus status) {
    std::cout << std::string{ "{ " }
        << std::string{ "document_id = " } << document_id << std::string{ ", " }
        << std::string{ "status = " } << static_cast<int>(status) << std::string{ ", " }
    << std::string{ "words =" };
    for (const std::string_view& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << std::string{ "}" } << std::endl;
}
