#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <execution>
#include <numeric>

//Отлично. Зачет. 

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            words.push_back(word);
            word.clear();
        } else {
            word += c;
        }
    }
    words.push_back(word);
    
    return words;
}
    


enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

struct Document {
    int id;
    double relevance;
    int rating;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }    
    
    void AddDocument(int document_id, const string& document,const DocumentStatus& status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, 
            DocumentData{
                ComputeAverageRating(ratings), 
                status
            });
    }
    
    template<typename DocumentsFilter>
    vector<Document> FindTopDocuments(const string& raw_query,DocumentsFilter document_filter ) const {            
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_filter);
        
        sort(execution::par,matched_documents.begin(), matched_documents.end(),               
             [](const Document& lhs, const Document& rhs) {
                return (abs(lhs.relevance - rhs.relevance) < 1e-6) ? (lhs.rating > rhs.rating) : (lhs.relevance > rhs.relevance);
             });
        
        if(matched_documents.size()>MAX_RESULT_DOCUMENT_COUNT){
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        

        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
    }
       
    
     vector<Document> FindTopDocuments(const string& raw_query,const DocumentStatus& status) const {            
        return FindTopDocuments(raw_query, [&status](int document_id, DocumentStatus status1, int rating) { return status1 == status; });
    }
    

    int GetDocumentCount() const {
        return documents_.size();
    }   
    
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (!IsContainWord(word)) {
                continue;
            }
            if (IsWordContainId(word, document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (!IsContainWord(word)) {
                continue;
            }
            if (IsWordContainId(word, document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
    
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    
    static int ComputeAverageRating(const vector<int>& ratings) {
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }
    
    bool IsContainWord(const string& word) const {
        return word_to_document_freqs_.count(word);
    }
    
    bool IsWordContainId(const string& word, const int doc_id) const{
        return word_to_document_freqs_.at(word).count(doc_id);
    }
    
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
    
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
    
    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-' && !text.empty()) {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }
   
    
    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            
            if (query_word.is_stop) {
                continue;
            }
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
        return query;
    }
    
    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
    
    template <typename Pred>
    vector<Document> FindAllDocuments(const Query& query, Pred pred) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if(pred(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)){
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating,
            });
        }
        return matched_documents;
    }
    
    
    
};

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestAddDocument() {                 // Тест функции AddDocument
    const int doc_id = 42;
    [[maybe_unused]] const int doc_id1 = 104;
    const string content = "cat in the city"s;
    const string content1 = "dog in the world"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        for (const string s : {"cat"s, "in"s, "the"s, "city"s}) {
            const auto found_docs = server.FindTopDocuments(s);
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id , doc_id);
        }
    }
}

void TestExcludedMinusWordsFromDocument() {
    const int doc_id = 42, doc_id1 = 104;
    const string content = "cat in cool city"s;
    const string content1 = "dog in perfect world"s;
    const vector<int> ratings = { 1, 2, 3 };

    {// проверка 1 документа
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u,"Size is not right"s);
        ASSERT_EQUAL_HINT(found_docs[0].id , doc_id,"Id is not right"s);
    }

    {// проверка 1 документа
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat -dog"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Size is not right"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id, "Id is not right"s);
    }

    {// проверка 1 документа
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat -city"s);
        ASSERT_HINT(found_docs.empty(),"found_docs must be empty!"s);
    }

    {// проверка c 2 документа
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        {
            const auto found_docs = server.FindTopDocuments("in"s);
            ASSERT_EQUAL_HINT(found_docs.size(), 2u, "Size is not right"s);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL_HINT(doc0.id , doc_id, "Id is not right"s);
            const Document& doc1 = found_docs[1];
            ASSERT_EQUAL_HINT(doc1.id, doc_id1, "Id is not right"s);
        }

        {
            const auto found_docs = server.FindTopDocuments("in -world"s);
            ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Size is not right"s);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL_HINT(doc0.id, doc_id, "Id is not right"s);
        }

        {
            const auto found_docs = server.FindTopDocuments("in -city"s);
            ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Size is not right"s);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL_HINT(doc0.id, doc_id1, "Id is not right"s);
        }

        {
            const auto found_docs = server.FindTopDocuments("in -in"s);
            ASSERT_HINT(found_docs.empty(), "found_docs must be empty!"s);
        }
    }
}

void TestMatchDocument() {
    const int doc_id = 42;
    [[maybe_unused]] const int doc_id1 = 104;
    const string content = "cat in cool city"s;
    const string content1 = "dog in cool world"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    {// проверка 1 документа
        const auto& [find_words, status] = server.MatchDocument("cat in cool city"s, doc_id);
        ASSERT_HINT(status == DocumentStatus::ACTUAL,"status must be ACTUAL"s);
        for (const auto& word : { "cat"s,"in"s,"cool"s,"city"s }) {
            ASSERT_HINT(count(find_words.begin(), find_words.end(), word), "find_words not full"s);
        }
        ASSERT_EQUAL_HINT(find_words.size(), 4u, "Size is not right"s);
    }
    {// проверка 1 документа
        const auto& [find_words, status] = server.MatchDocument("cat bad giy city"s, doc_id);
        ASSERT_HINT(status == DocumentStatus::ACTUAL, "status must be ACTUAL"s);
        for (const auto& word : { "cat"s,"city"s }) {
            ASSERT_HINT(count(find_words.begin(), find_words.end(), word), "find_words not full"s);
        }
        ASSERT_EQUAL_HINT(find_words.size(), 2u, "Size is not right"s);
    }

    {// проверка 1 документа
        const auto& [find_words, status] = server.MatchDocument("dog life is best life"s, doc_id);
        ASSERT_HINT(status == DocumentStatus::ACTUAL, "status must be ACTUAL"s);
        ASSERT_HINT(find_words.empty(), "found_docs must be empty!"s);
    }
    {// проверка 1 документа
        const auto& [find_words, status] = server.MatchDocument(""s, doc_id);
        ASSERT_HINT(status == DocumentStatus::ACTUAL, "status must be ACTUAL"s);
        ASSERT_HINT(find_words.empty(), "found_docs must be empty!"s);
    }
}

void TestFindTopDocumentsSortByRelevance() {
    const int doc_id = 42;
    const int doc_id1 = 104;
    const int doc_id2 = 950;
    const int doc_id3 = 272;
    const int doc_id4 = 1080;
    const int doc_id5 = 228;
    const string content = "cat in cool city"s;
    const string content1 = "dog in cool world"s;
    const string content2 = "throw duck in a river"s;
    const string content3 = "i am so good dog"s;
    const string content4 = "king of the hill"s;
    const string content5 = "my room is good anouth"s;
    const vector<int> ratings = { 1, 2, 3 };


    {// проверка 5 документа
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat dog"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 3u, "Size is not right"s);

        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id, "Id is not right"s);
        ASSERT_EQUAL_HINT(found_docs[1].id, doc_id1, "Id is not right"s);
        ASSERT_EQUAL_HINT(found_docs[2].id, doc_id3, "Id is not right"s);
    }

    {// проверка 5 документа
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat dog duck good hill"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 5u, "Size is not right"s);

        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id3, "Id is not right"s);
        ASSERT_EQUAL_HINT(found_docs[1].id, doc_id, "Id is not right"s);
        ASSERT_EQUAL_HINT(found_docs[2].id, doc_id4, "Id is not right"s);
        ASSERT_EQUAL_HINT(found_docs[3].id, doc_id2, "Id is not right"s);
        ASSERT_EQUAL_HINT(found_docs[4].id, doc_id1, "Id is not right"s);
    }

    {// проверка 6 документа
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id5, content5, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat dog duck good hill"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 5u, "Size is not right"s);

        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id, "Id is not right"s);
        ASSERT_EQUAL_HINT(found_docs[1].id, doc_id4, "Id is not right"s);
        ASSERT_EQUAL_HINT(found_docs[2].id, doc_id3, "Id is not right"s);
        ASSERT_EQUAL_HINT(found_docs[3].id, doc_id2, "Id is not right"s);
        ASSERT_EQUAL_HINT(found_docs[4].id, doc_id1, "Id is not right"s);

    }
}

void TestCalculateRating() {
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const int doc_id4 = 4;
    const string content = "cat in cool city"s;
    const vector<int> ratings1 = { 1, 2, 3 };
    const vector<int> ratings2 = { 4, 28, -16 };
    const vector<int> ratings3 = { 4, -16, 3 };
    const vector<int> ratings4 = { 1, 3, 1 };

    SearchServer server;
    server.AddDocument(doc_id1, content, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(doc_id3, content, DocumentStatus::ACTUAL, ratings3);
    server.AddDocument(doc_id4, content, DocumentStatus::ACTUAL, ratings4);

    auto ComputeAvarageRating = [](const vector<int> ratings) {
        const auto sum = accumulate(ratings.begin(), ratings.end(), 0);
        return sum * 1 / static_cast<int>(ratings.size());
    };

    auto found_docs = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL_HINT(found_docs.size(), 4u, "Size is not right"s);

    sort(found_docs.begin(), found_docs.end(), [](const Document& lhs, const Document& rhs) {
        return lhs.id < rhs.id;
        });

    ASSERT_EQUAL_HINT(found_docs[0].rating, ComputeAvarageRating(ratings1), "rating is not right"s);
    ASSERT_EQUAL_HINT(found_docs[1].rating, ComputeAvarageRating(ratings2), "rating is not right"s);
    ASSERT_EQUAL_HINT(found_docs[2].rating, ComputeAvarageRating(ratings3), "rating is not right"s);
    ASSERT_EQUAL_HINT(found_docs[3].rating, ComputeAvarageRating(ratings4), "rating is not right"s);


}

void TestFindTopWithPredicatStatus() {
    const int doc_id = 42;
    const int doc_id1 = 104;
    const int doc_id2 = 950;
    const int doc_id3 = 272;
    const string content = "cat in cool city"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id1, content, DocumentStatus::REMOVED, ratings);
    server.AddDocument(doc_id2, content, DocumentStatus::BANNED, ratings);
    server.AddDocument(doc_id3, content, DocumentStatus::IRRELEVANT, ratings);


    {// проверка 4 документа
        const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::BANNED);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Size is not right"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id2, "Id is not right"s);
    }

    {// проверка 4 документа
        const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Size is not right"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id1, "Id is not right"s);
    }

    {// проверка 4 документа
        const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Size is not right"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id, "Id is not right"s);
    }

    {// проверка 4 документа
        const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Size is not right"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id3, "Id is not right"s);
    }

}


void TestFindTopWithPredicat() {
    const int doc_id1 = 42;
    [[maybe_unused]] const int doc_id2 = 104;
    [[maybe_unused]] const int doc_id3 = 950;
    [[maybe_unused]] const int doc_id4 = 272;
    const string content = "cat in cool city"s;
    const vector<int> ratings1 = { 1, 2, 3 };       // 2
    const vector<int> ratings2 = { 4, 28, -16 };     //5
    const vector<int> ratings3 = { 4, -16, 3 };      //-3
    const vector<int> ratings4 = { 1, 3, 1 };        //1

    SearchServer server;

    {// проверка 0 документа
        const auto found_docs = server.FindTopDocuments("cat dog"s, [](int id, DocumentStatus status, int rating) {return rating > 5; });
        ASSERT_HINT(found_docs.empty(),"found_docs must be empty"s);
    }

    {// проверка 0 документа
        const auto found_docs = server.FindTopDocuments(""s, [](int id, DocumentStatus status, int rating) {return rating > 5; });
        ASSERT_HINT(found_docs.empty(), "found_docs must be empty"s);
    }
    server.AddDocument(doc_id1, content, DocumentStatus::ACTUAL, ratings1);

    {// проверка 1 документа
        const auto found_docs = server.FindTopDocuments("cat dog"s, [](int id, DocumentStatus status, int rating) {return rating > 5; });
        ASSERT_HINT(found_docs.empty(), "found_docs must be empty"s);
    }

    {// проверка 1 документа
        const auto found_docs = server.FindTopDocuments("cat dog"s, [](int id, DocumentStatus status, int rating) {return rating <= 5; });
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Size is not right"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id1, "Id is not right"s);
    }

    {// проверка 1 документа
        const auto found_docs = server.FindTopDocuments("cat dog"s, [](int id, DocumentStatus status, int rating) {return status != DocumentStatus::ACTUAL; });
        ASSERT_HINT(found_docs.empty(), "found_docs must be empty"s);
    }

    {// проверка 1 документа
        const auto found_docs = server.FindTopDocuments("cat dog"s, [](int id, DocumentStatus status, int rating) {return status == DocumentStatus::ACTUAL; });
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Size is not right"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id1, "Id is not right"s);
    }

    {// проверка 1 документа
        const auto found_docs = server.FindTopDocuments("cat dog"s, [doc_id1](int id, DocumentStatus status, int rating) {return id != doc_id1; });
        ASSERT_HINT(found_docs.empty(), "found_docs must be empty"s);
    }

    {// проверка 1 документа
        const auto found_docs = server.FindTopDocuments("cat dog"s, [doc_id1](int id, DocumentStatus status, int rating) {return id == doc_id1;  });
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Size is not right"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id1, "Id is not right"s);
    }

}



void TestRelevanceCalculationIdf() {
    SearchServer server;
    const int startDocId = 51;

    int docId = startDocId;
    {
        const std::string content = "cat and dog"s;
        server.AddDocument(docId, content, DocumentStatus::ACTUAL, { 1 });
    }
    ++docId;
    {
        const std::string content = "cat and cow"s;
        server.AddDocument(docId, content, DocumentStatus::ACTUAL, { 1 });
    }
    ++docId;
    {
        const std::string content = "city"s;
        server.AddDocument(docId, content, DocumentStatus::ACTUAL, { 1 });
    }

    auto calculateRelevance = [](const int totalDocumentsCount,
        const int documentsContainingTheWordCount,
        const int wordInDocumentCount,
        const int documentSize) {
            double tf = double(wordInDocumentCount) / documentSize;
            double idf =
                log(totalDocumentsCount * 1. / documentsContainingTheWordCount);
            return tf * idf;
    };

    {
        auto foundDocs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL_HINT(foundDocs.size(), 2u, "Size is not right"s);
        std::sort(foundDocs.begin(), foundDocs.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.id < rhs.id;
            });

        ASSERT_EQUAL_HINT(foundDocs[0].relevance, calculateRelevance(3, 2, 1, 3), "relevance calculattion is not right"s);
        ASSERT_EQUAL_HINT(foundDocs[1].relevance, calculateRelevance(3, 2, 1, 3), "relevance calculattion is not right"s);
    }
    {
        auto foundDocs = server.FindTopDocuments("dog"s);
        ASSERT_EQUAL_HINT(foundDocs.size(), 1u, "Size is not right"s);
        ASSERT_EQUAL_HINT(foundDocs[0].relevance, calculateRelevance(3, 1, 1, 3), "relevance calculattion is not right"s);
    }
}

void TestRelevanceCalculationTf() {
    SearchServer server;
    const int startDocId = 51;

    int docId = startDocId;
    {
        const std::string content = "cat and dog"s;
        server.AddDocument(docId, content, DocumentStatus::ACTUAL, { 1 });
    }
    ++docId;
    {
        const std::string content = "cat and cat"s;
        server.AddDocument(docId, content, DocumentStatus::ACTUAL, { 1 });
    }
    ++docId;
    {
        const std::string content = "city"s;
        server.AddDocument(docId, content, DocumentStatus::ACTUAL, { 1 });
    }

    auto calculateRelevance = [](const int totalDocumentsCount,
        const int documentsContainingTheWordCount,
        const int wordInDocumentCount,
        const int documentSize) {
            double tf = double(wordInDocumentCount) / documentSize;
            double idf =
                log(totalDocumentsCount * 1. / documentsContainingTheWordCount);
            return tf * idf;
    };

    {
        auto foundDocs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL_HINT(foundDocs.size(), 2u, "Size is not right"s);
        std::sort(foundDocs.begin(), foundDocs.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.id < rhs.id;
            });

        ASSERT_EQUAL_HINT(foundDocs[0].relevance, calculateRelevance(3, 2, 1, 3), "relevance calculattion is not right"s);
        ASSERT_EQUAL_HINT(foundDocs[1].relevance, calculateRelevance(3, 2, 2, 3), "relevance calculattion is not right"s);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestExcludedMinusWordsFromDocument);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestFindTopDocumentsSortByRelevance);
    RUN_TEST(TestCalculateRating);
    RUN_TEST(TestFindTopWithPredicatStatus);
    RUN_TEST(TestRelevanceCalculationIdf);
    RUN_TEST(TestRelevanceCalculationTf);
}


// ==================== для примера =========================

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() {
    void TestSearchServer() ;
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }

    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    auto Pred = [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; };
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, Pred)) {
        PrintDocument(document);
    }

    return 0;
} 
