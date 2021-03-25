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
    string word = ""s;
    for (const char c : text) {
        if (c != ' ') {
            word += c;
        }
        else if (!word.empty()) {
            words.push_back(word);
            word.clear();
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
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};


template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

class SearchServer {
public:

    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (const string& word : stop_words) {
            if (CheckingForSpecialSymbols(word)) {
                throw invalid_argument("sodergit spec sumbols"s);
            }
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
        if (CheckingForSpecialSymbols(stop_words_text)) {
            throw invalid_argument("sodergit spec sumbols"s);
        }
    }

    void AddDocument(int document_id, const string& document, const DocumentStatus& status, const vector<int>& ratings) {
        if (document_id < 0 || documents_.count(document_id) || CheckingForSpecialSymbols(document)) {
            throw invalid_argument("something"s);
        }

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
        document_ids_.push_back(document_id);
    }

    template<typename DocumentsFilter>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentsFilter document_filter) const {
        if (CheckingForSpecialSymbols(raw_query) || CheckingForEmptyMinusWord(raw_query)) {
            throw invalid_argument("something"s);
        }

        const Query query = ParseQuery(raw_query);

        if (CheckingForDoubleMinus(query.minus_words)) {
            throw invalid_argument("something"s);
        }

        auto matched_documents = FindAllDocuments(query, document_filter);

        sort(execution::par, matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
            return (abs(lhs.relevance - rhs.relevance) < 1e-6) ? (lhs.rating > rhs.rating) : (lhs.relevance > rhs.relevance);
        });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }


        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
    }


    vector<Document> FindTopDocuments(const string& raw_query, const DocumentStatus& status) const {
        return FindTopDocuments(raw_query, [&status](int document_id, DocumentStatus status1, int rating) { return status1 == status; });
    }


    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        if (CheckingForSpecialSymbols(raw_query) || CheckingForEmptyMinusWord(raw_query)) {
            throw invalid_argument("something"s);
        }

        const Query query = ParseQuery(raw_query);

        if (CheckingForDoubleMinus(query.minus_words)) {
            throw invalid_argument("something"s);
        }

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
        return { matched_words, documents_.at(document_id).status };
    }

    int GetDocumentId(int index) const {
        if (index >= 0 && index < GetDocumentCount()) {
            return document_ids_[index];
        }
        throw out_of_range("probleva"s);
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
    vector<int> document_ids_;


    bool CheckingForEmptyMinusWord(const string& s) const {
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

    bool CheckingForDoubleMinus(const set<string>& words) const {
        for (const string& word : words) {
            if (word[0] == '-') {
                return true;
            }
        }
        return false;
    }

    bool CheckingForSpecialSymbols(const string& s) const {
        for (const char c : s) {
            if (c >= 0 && c <= 31)
                return true;
        }
        return false;
    }

    template<typename T>                     // для vector и set
    void SetStopWords(const T& container) {
        for (const string& word : container) {
            if (word != ""s) {
                stop_words_.insert(word);
            }
        }
    }

    void SetStopWords(const string& text) {                // для string
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

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

    bool IsWordContainId(const string& word, const int doc_id) const {
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
            }
            else {
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
                if (pred(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
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
                                          // tests

template <typename Type>
ostream& operator<<(ostream& out, const vector<Type> V) {
    out << "["s;
    bool b = false;
    for (const Type& temp : V) {
        if (b) {
            out << ", "s;
        }
        b = true;
        out << temp;
    }
    out << "]"s;
    return out;
}

template <typename Type>
ostream& operator<<(ostream& out, const set<Type> S) {
    out << "{"s;
    bool b = false;
    for (const Type& temp : S) {
        if (b) {
            out << ", "s;
        }
        b = true;
        out << temp;
    }
    out << "}"s;
    return out;
}

template <typename Key, typename Value>
ostream& operator<<(ostream& out, const map<Key, Value> M) {
    out << "{"s;
    bool b = false;
    for (const auto& [key, value] : M) {
        if (b) {
            out << ", "s;
        }
        b = true;
        out << key << ": "s << value;
    }
    out << "}"s;
    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

void AssertEqualImpl(const double t, const double u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (abs(t - u) > 1e-6) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

#define RUN_TEST(func)  func()
#define RUN_TEST_WITH_ARG(func) func

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);

        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL(found_docs[0].id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestAddDocument() {                 // Тест функции AddDocument
    SearchServer server(""s);
    const int doc_id = 42;
    server.AddDocument(doc_id, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto found_docs1 = server.FindTopDocuments("cat"s);
    const auto found_docs2 = server.FindTopDocuments("in"s);
    const auto found_docs3 = server.FindTopDocuments("the"s);
    const auto found_docs4 = server.FindTopDocuments("city"s);

    ASSERT_EQUAL(found_docs1.size(), 1u);
    ASSERT_EQUAL(found_docs1[0].id, doc_id);
    ASSERT_EQUAL(found_docs2.size(), 1u);
    ASSERT_EQUAL(found_docs2[0].id, doc_id);
    ASSERT_EQUAL(found_docs3.size(), 1u);
    ASSERT_EQUAL(found_docs3[0].id, doc_id);
    ASSERT_EQUAL(found_docs4.size(), 1u);
    ASSERT_EQUAL(found_docs4[0].id, doc_id);
}

void TestExcludedMinusWordsWithOneDocument(const string& query) {
    SearchServer server(""s);
    server.AddDocument(42, "cat in cool city"s, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments(query);

    ASSERT_EQUAL(found_docs.size(), 1u);
    ASSERT_EQUAL(found_docs[0].id, 42);
}

void TestExcludedMinusWordsWithOneDocument_ResultEmpty() {
    SearchServer server(""s);
    server.AddDocument(42, "cat in cool city"s, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments("cat -city"s);

    ASSERT_HINT(found_docs.empty(), "found_docs must be empty!"s);
}

void TestExcludedMinusWordsFromDocument_Empty() {
    SearchServer server(""s);
    server.AddDocument(42, "cat in cool city"s, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(104, "dog in perfect world"s, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments("in -in"s);

    ASSERT_HINT(found_docs.empty(), "found_docs must be empty!"s);
}

void TestExcludedMinusWordsFromDocument_OneAnswer(const string& query, int etalonID) {
    SearchServer server(""s);
    server.AddDocument(42, "cat in cool city"s, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(104, "dog in perfect world"s, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments(query);

    ASSERT_EQUAL(found_docs.size(), 1u);
    ASSERT_EQUAL(found_docs[0].id, etalonID);
}

void TestExcludedMinusWordsFromDocument_TwoAnswer() {
    const int doc_id = 42, doc_id1 = 104;
    SearchServer server(""s);
    server.AddDocument(doc_id, "cat in cool city"s, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(doc_id1, "dog in perfect world"s, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments("in"s);

    ASSERT_EQUAL(found_docs.size(), 2u);
    ASSERT_EQUAL(found_docs[0].id, doc_id);
    ASSERT_EQUAL(found_docs[1].id, doc_id1);
}

void TestMatchDocument() {
    const int doc_id = 42;
    SearchServer server(""s);
    server.AddDocument(doc_id, "cat in cool city"s, DocumentStatus::ACTUAL, { 1 });
    const string word1 = "cat"s;
    const string word2 = "in"s;
    const string word3 = "cool"s;
    const string word4 = "city"s;

    {// проверка 1 документа
        const auto& [find_words, status] = server.MatchDocument("cat in cool city"s, doc_id);

        ASSERT_HINT(status == DocumentStatus::ACTUAL, "status must be ACTUAL"s);
        ASSERT_HINT(count(find_words.begin(), find_words.end(), word1), "no word cat in find_words"s);
        ASSERT_HINT(count(find_words.begin(), find_words.end(), word2), "no word in in find_words"s);
        ASSERT_HINT(count(find_words.begin(), find_words.end(), word3), "no word cool in find_words"s);
        ASSERT_HINT(count(find_words.begin(), find_words.end(), word4), "no word city in find_words"s);
        ASSERT_EQUAL(find_words.size(), 4u);
    }
    {// проверка 1 документа
        const auto& [find_words, status] = server.MatchDocument("cat bad giy city"s, doc_id);

        ASSERT_HINT(status == DocumentStatus::ACTUAL, "status must be ACTUAL"s);
        ASSERT_HINT(count(find_words.begin(), find_words.end(), word1), "no word cat in find_words"s);
        ASSERT_HINT(count(find_words.begin(), find_words.end(), word4), "no word city in find_words"s);
        ASSERT_EQUAL(find_words.size(), 2u);
    }

    {// проверка 1 документа
        const auto& [find_words, status] = server.MatchDocument("dog life is best life"s, doc_id);

        ASSERT_HINT(status == DocumentStatus::ACTUAL, "status must be ACTUAL"s);
        ASSERT_HINT(find_words.empty(), "find_words must be empty!"s);
    }
    {// проверка 1 документа
        const auto& [find_words, status] = server.MatchDocument(""s, doc_id);

        ASSERT_HINT(status == DocumentStatus::ACTUAL, "status must be ACTUAL"s);
        ASSERT_HINT(find_words.empty(), "find_words must be empty!"s);
    }
}

SearchServer creating_a_filled_object(int id, int id1, int id2, int id3, int id4) {
    SearchServer server(""s);
    server.AddDocument(id, "cat in cool city"s, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(id1, "dog in cool world"s, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(id2, "throw duck in a river"s, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(id3, "i am so good dog"s, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(id4, "king of the hill"s, DocumentStatus::ACTUAL, { 1 });
    return server;
}

void TestFindTopDocumentsSortByRelevance() {
    const int doc_id = 42;
    const int doc_id1 = 104;
    const int doc_id2 = 950;
    const int doc_id3 = 272;
    const int doc_id4 = 1080;
    const int doc_id5 = 228;
    SearchServer server = creating_a_filled_object(doc_id, doc_id1, doc_id2, doc_id3, doc_id4);   //метод, возвращающий сконструированный объект SearchServer

    {// проверка 5 документа                                                      
        const auto found_docs = server.FindTopDocuments("cat dog"s);

        ASSERT_EQUAL(found_docs.size(), 3u);
        ASSERT_EQUAL(found_docs[0].id, doc_id);
        ASSERT_EQUAL(found_docs[1].id, doc_id1);
        ASSERT_EQUAL(found_docs[2].id, doc_id3);
    }

    {// проверка 5 документа
        const auto found_docs = server.FindTopDocuments("cat dog duck good hill"s);

        ASSERT_EQUAL(found_docs.size(), 5u);
        ASSERT_EQUAL(found_docs[0].id, doc_id3);
        ASSERT_EQUAL(found_docs[1].id, doc_id);
        ASSERT_EQUAL(found_docs[2].id, doc_id4);
        ASSERT_EQUAL(found_docs[3].id, doc_id2);
        ASSERT_EQUAL(found_docs[4].id, doc_id1);
    }

    {// проверка 6 документа
        server.AddDocument(doc_id5, "my room is good anouth"s, DocumentStatus::ACTUAL, { 1 });
        const auto found_docs = server.FindTopDocuments("cat dog duck good hill"s);

        ASSERT_EQUAL(found_docs.size(), 5u);
        ASSERT_EQUAL(found_docs[0].id, doc_id);
        ASSERT_EQUAL(found_docs[1].id, doc_id4);
        ASSERT_EQUAL(found_docs[2].id, doc_id3);
        ASSERT_EQUAL(found_docs[3].id, doc_id2);
        ASSERT_EQUAL(found_docs[4].id, doc_id1);
    }
}

void TestCalculateRating() {
    const string content = "cat in cool city"s;
    const int arithmetic_mean_document_1 = 2;
    const int arithmetic_mean_document_2 = 5;
    const int arithmetic_mean_document_3 = -3;
    const int arithmetic_mean_document_4 = 1;
    SearchServer server(""s);
    server.AddDocument(1, content, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, content, DocumentStatus::ACTUAL, { 4, 28, -16 });
    server.AddDocument(3, content, DocumentStatus::ACTUAL, { 4, -16, 3 });
    server.AddDocument(4, content, DocumentStatus::ACTUAL, { 1, 3, 1 });
    auto found_docs = server.FindTopDocuments("cat"s);
    sort(found_docs.begin(), found_docs.end(), [](const Document& lhs, const Document& rhs) {
        return lhs.id < rhs.id;
    });

    ASSERT_EQUAL(found_docs.size(), 4u);
    ASSERT_EQUAL(found_docs[0].rating, arithmetic_mean_document_1);
    ASSERT_EQUAL(found_docs[1].rating, arithmetic_mean_document_2);
    ASSERT_EQUAL(found_docs[2].rating, arithmetic_mean_document_3);
    ASSERT_EQUAL(found_docs[3].rating, arithmetic_mean_document_4);
}

void TestFindTopWithStatus(DocumentStatus status, int etalonId)
{
    const int doc_id = 42;
    const int doc_id1 = 104;
    const int doc_id2 = 950;
    const int doc_id3 = 272;
    const string content = "cat in cool city"s;
    const vector ratings = { 1 };
    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id1, content, DocumentStatus::REMOVED, ratings);
    server.AddDocument(doc_id2, content, DocumentStatus::BANNED, ratings);
    server.AddDocument(doc_id3, content, DocumentStatus::IRRELEVANT, ratings);

    const auto found_docs = server.FindTopDocuments("cat"s, status);

    ASSERT_EQUAL(found_docs.size(), 1u);
    ASSERT_EQUAL(found_docs[0].id, etalonId);
}

template<typename Func>
void TestFindTopWithPredicatAndEmptySearchServer_ResultEmpty(Func predicate, const string& query)
{
    SearchServer server(""s);

    const auto found_docs = server.FindTopDocuments("cat dog"s, predicate);

    ASSERT(found_docs.empty());
}

template<typename Func>
void TestFindTopWithPredicat_ResultEmpty(Func predicate)
{
    const int doc_id1 = 42;
    SearchServer server(""s);
    server.AddDocument(doc_id1, "cat in cool city"s, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments("cat dog"s, predicate);

    ASSERT(found_docs.empty());
}

template<typename Func>
void TestFindTopWithPredicat(Func predicate)
{
    const int doc_id1 = 42;
    SearchServer server(""s);
    server.AddDocument(doc_id1, "cat in cool city"s, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments("cat dog"s, predicate);

    ASSERT_EQUAL(found_docs.size(), 1u);
    ASSERT_EQUAL(found_docs[0].id, doc_id1);
}

void TestRelevanceCalculation() {
    SearchServer server(""s);
    const double relevance_doc_1_and_2 = 0.135155;
    const double relevance_doc_3 = 0.366204;
    server.AddDocument(1, "cat and dog"s, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(2, "cat and cow"s, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(3, "city"s, DocumentStatus::ACTUAL, { 1 });

    {
        auto foundDocs = server.FindTopDocuments("cat"s);
        std::sort(foundDocs.begin(), foundDocs.end(),
            [](const Document& lhs, const Document& rhs) {
            return lhs.id < rhs.id;
        });

        ASSERT_EQUAL(foundDocs.size(), 2u);
        ASSERT_EQUAL(foundDocs[0].relevance, relevance_doc_1_and_2);  // частный случай метода, когда входные данные типа double
        ASSERT_EQUAL(foundDocs[1].relevance, relevance_doc_1_and_2);
    }
    {
        auto foundDocs = server.FindTopDocuments("dog"s);

        ASSERT_EQUAL(foundDocs.size(), 1u, "Size is not right"s);
        ASSERT_EQUAL(foundDocs[0].relevance, relevance_doc_3);
    }
}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestExcludedMinusWordsWithOneDocument_ResultEmpty);
    RUN_TEST_WITH_ARG(TestExcludedMinusWordsWithOneDocument("cat"s));
    RUN_TEST_WITH_ARG(TestExcludedMinusWordsWithOneDocument("cat -dog"s));
    RUN_TEST_WITH_ARG(TestExcludedMinusWordsFromDocument_OneAnswer("in -world"s, 42));
    RUN_TEST_WITH_ARG(TestExcludedMinusWordsFromDocument_OneAnswer("in -city"s, 104));
    RUN_TEST(TestExcludedMinusWordsFromDocument_TwoAnswer);
    RUN_TEST(TestExcludedMinusWordsFromDocument_Empty);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestFindTopDocumentsSortByRelevance);
    RUN_TEST(TestCalculateRating);
    RUN_TEST_WITH_ARG(TestFindTopWithStatus(DocumentStatus::ACTUAL, 42));
    RUN_TEST_WITH_ARG(TestFindTopWithStatus(DocumentStatus::BANNED, 950));
    RUN_TEST_WITH_ARG(TestFindTopWithStatus(DocumentStatus::IRRELEVANT, 272));
    RUN_TEST_WITH_ARG(TestFindTopWithStatus(DocumentStatus::REMOVED, 104));
    RUN_TEST_WITH_ARG(TestFindTopWithPredicatAndEmptySearchServer_ResultEmpty([](int id, DocumentStatus status, int rating) {return rating > 5; }, "cat dog"s));
    RUN_TEST_WITH_ARG(TestFindTopWithPredicatAndEmptySearchServer_ResultEmpty([](int id, DocumentStatus status, int rating) {return rating > 5; }, ""s));
    RUN_TEST_WITH_ARG(TestFindTopWithPredicat_ResultEmpty([](int id, DocumentStatus status, int rating) {return rating > 5; }));
    RUN_TEST_WITH_ARG(TestFindTopWithPredicat_ResultEmpty([](int id, DocumentStatus status, int rating) {return status != DocumentStatus::ACTUAL; }));
    RUN_TEST_WITH_ARG(TestFindTopWithPredicat_ResultEmpty([](int id, DocumentStatus status, int rating) {return id != 42; }));
    RUN_TEST_WITH_ARG(TestFindTopWithPredicat([](int id, DocumentStatus status, int rating) {return rating <= 5; }));
    RUN_TEST_WITH_ARG(TestFindTopWithPredicat([](int id, DocumentStatus status, int rating) {return status == DocumentStatus::ACTUAL; }));
    RUN_TEST_WITH_ARG(TestFindTopWithPredicat([](int id, DocumentStatus status, int rating) {return id == 42;  }));
    RUN_TEST(TestRelevanceCalculation);
}


// ------------ Пример использования ----------------

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast<int>(status) << ", "s
        << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
    const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const exception & e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const exception & e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const exception & e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}

int main() {
    setlocale(LC_ALL, "Russian");
    SearchServer search_server("и в на"s);

    AddDocument(search_server, 1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    AddDocument(search_server, 1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
    AddDocument(search_server, -1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
    AddDocument(search_server, 3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    AddDocument(search_server, 4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

    FindTopDocuments(search_server, "пушистый -пёс"s);
    FindTopDocuments(search_server, "пушистый --кот"s);
    FindTopDocuments(search_server, "пушистый -"s);

    MatchDocuments(search_server, "пушистый пёс"s);
    MatchDocuments(search_server, "модный -кот"s);
    MatchDocuments(search_server, "модный --пёс"s);
    MatchDocuments(search_server, "пушистый - хвост"s);

    system("pause");
}
