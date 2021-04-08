#include "search_server_tests.h"

#include <execution>

void AddDocumentTest() {                 // Тест функции AddDocument
    SearchServer server(std::string{ "" });
    const int doc_id = 42;
    server.AddDocument(doc_id, std::string{ "cat in the city" }, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto found_docs1 = server.FindTopDocuments(std::string{ "cat" });
    const auto found_docs2 = server.FindTopDocuments(std::string{ "in" });
    const auto found_docs3 = server.FindTopDocuments(std::string{ "the" });
    const auto found_docs4 = server.FindTopDocuments(std::string{ "city" });

    ASSERT_EQUAL(found_docs1.size(), 1u);
    ASSERT_EQUAL(found_docs1[0].id, doc_id);
    ASSERT_EQUAL(found_docs2.size(), 1u);
    ASSERT_EQUAL(found_docs2[0].id, doc_id);
    ASSERT_EQUAL(found_docs3.size(), 1u);
    ASSERT_EQUAL(found_docs3[0].id, doc_id);
    ASSERT_EQUAL(found_docs4.size(), 1u);
    ASSERT_EQUAL(found_docs4[0].id, doc_id);
}

void Test_ExcludeStopWords_FromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = std::string{ "cat in the city" };
    const std::vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server(std::string{ "in the" });
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        ASSERT(server.FindTopDocuments(std::string{ "in" }).empty());
    }
}

void Test_ExcludedMinusWords_WithOneDocument_ResultFindDocument(const std::string& query) {
    SearchServer server(std::string{ "" });
    server.AddDocument(42, std::string{ "cat in cool city" }, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments(query);

    ASSERT_EQUAL(found_docs.size(), 1u);
    ASSERT_EQUAL(found_docs[0].id, 42);
}

void TestExcludedMinusWords_WithOneDocument_ResultEmpty() {
    SearchServer server(std::string{ "" });
    server.AddDocument(42, std::string{ "cat in cool city" }, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments(std::string{ "cat -city" });

    ASSERT(found_docs.empty());
}

void TestExcludedMinusWords_FromTwoDocuments_ResultEmpty() {
    SearchServer server(std::string{ "" });
    server.AddDocument(42, std::string{ "cat in cool city" }, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(104, std::string{ "dog in perfect world" }, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments(std::string{ "in -in" });

    ASSERT(found_docs.empty());
}

void TestExcludedMinusWords_FromTwoDocument_ResultOneDocument(const std::string& query, int etalonID) {
    SearchServer server(std::string{ "" });
    server.AddDocument(42, std::string{ "cat in cool city" }, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(104, std::string{ "dog in perfect world" }, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments(query);

    ASSERT_EQUAL(found_docs.size(), 1u);
    ASSERT_EQUAL(found_docs[0].id, etalonID);
}

void TestExcludedMinusWords_FromTwoDocuments_ResultTwoDocuments() {
    const int doc_id = 42, doc_id1 = 104;
    SearchServer server(std::string{ "" });
    server.AddDocument(doc_id, std::string{ "cat in cool city" }, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(doc_id1, std::string{ "dog in perfect world" }, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments(std::string{ "in" });

    ASSERT_EQUAL(found_docs.size(), 2u);
    ASSERT_EQUAL(found_docs[0].id, doc_id);
    ASSERT_EQUAL(found_docs[1].id, doc_id1);
}

void Test_GetDocumentCount_ResultEmpty() {
    const int doc_id = 42, doc_id1 = 104;
    SearchServer server(std::string{ "" });
    
    int document_count = server.GetDocumentCount();

    ASSERT_EQUAL(document_count, 0);
}

void Test_GetDocumentCount_ResultThreeDocuments() {
    const int doc_id = 42, doc_id1 = 104, doc_id2 = 210;
    SearchServer server(std::string{ "" });
    server.AddDocument(doc_id, std::string{ "cat in cool city" }, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(doc_id1, std::string{ "dog in perfect world" }, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(doc_id2, std::string{ "dog life is best life" }, DocumentStatus::ACTUAL, { 1 });

    int document_count = server.GetDocumentCount();

    ASSERT_EQUAL(document_count, 3);
}

void Test_GetDocumentId() {
    const int doc_id = 42, doc_id1 = 104, doc_id2 = 210;
    SearchServer server(std::string{ "" });
    server.AddDocument(doc_id, std::string{ "cat in cool city" }, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(doc_id1, std::string{ "dog in perfect world" }, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(doc_id2, std::string{ "dog life is best life" }, DocumentStatus::ACTUAL, { 1 });

    int document_id1 = server.GetDocumentId(0);
    int document_id2 = server.GetDocumentId(1);
    int document_id3 = server.GetDocumentId(2);

    ASSERT_EQUAL(document_id1, doc_id);
    ASSERT_EQUAL(document_id2, doc_id1);
    ASSERT_EQUAL(document_id3, doc_id2);
}

void TestMatchDocument() {
    const int doc_id = 42;
    SearchServer server(std::string{ "" });
    server.AddDocument(doc_id, std::string{ "cat in cool city" }, DocumentStatus::ACTUAL, { 1 });
    const std::string word1 = std::string{ "cat" };
    const std::string word2 = std::string{ "in" };
    const std::string word3 = std::string{ "cool" };
    const std::string word4 = std::string{ "city" };

    {// проверка 1 документа
        const auto& [find_words, status] = server.MatchDocument(std::string{ "cat in cool city" }, doc_id);

        ASSERT(status == DocumentStatus::ACTUAL);
        ASSERT(count(find_words.begin(), find_words.end(), word1));
        ASSERT(count(find_words.begin(), find_words.end(), word2));
        ASSERT(count(find_words.begin(), find_words.end(), word3));
        ASSERT(count(find_words.begin(), find_words.end(), word4));
        ASSERT_EQUAL(find_words.size(), 4u);
    }
    {// проверка 1 документа
        const auto& [find_words, status] = server.MatchDocument(std::string{ "cat bad giy city" }, doc_id);

        ASSERT(status == DocumentStatus::ACTUAL);
        ASSERT(count(find_words.begin(), find_words.end(), word1));
        ASSERT(count(find_words.begin(), find_words.end(), word4));
        ASSERT_EQUAL(find_words.size(), 2u);
    }

    {// проверка 1 документа
        const auto& [find_words, status] = server.MatchDocument(std::string{ "dog life is best life" }, doc_id);

        ASSERT(status == DocumentStatus::ACTUAL);
        ASSERT(find_words.empty());
    }
    {// проверка 1 документа
        const auto& [find_words, status] = server.MatchDocument(std::string{ "" }, doc_id);

        ASSERT(status == DocumentStatus::ACTUAL);
        ASSERT(find_words.empty());
    }
}

SearchServer Сreating_Filled_Object(int id, int id1, int id2, int id3, int id4) {
    SearchServer server(std::string{ "" });
    server.AddDocument(id, std::string{ "cat in cool city" }, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(id1, std::string{ "dog in cool world" }, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(id2, std::string{ "throw duck in a river" }, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(id3, std::string{ "i am so good dog" }, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(id4, std::string{ "king of the hill" }, DocumentStatus::ACTUAL, { 1 });
    return server;
}

void Test_FindTopDocuments_SortByRelevance() {
    const int doc_id = 42;
    const int doc_id1 = 104;
    const int doc_id2 = 950;
    const int doc_id3 = 272;
    const int doc_id4 = 1080;
    const int doc_id5 = 228;
    SearchServer server = Сreating_Filled_Object(doc_id, doc_id1, doc_id2, doc_id3, doc_id4);   //метод, возвращающий сконструированный объект SearchServer

    {// проверка 5 документа                                                      
        const auto found_docs = server.FindTopDocuments(std::string{ "cat dog" });

        ASSERT_EQUAL(found_docs.size(), 3u);
        ASSERT_EQUAL(found_docs[0].id, doc_id);
        ASSERT_EQUAL(found_docs[1].id, doc_id1);
        ASSERT_EQUAL(found_docs[2].id, doc_id3);
    }

    {// проверка 5 документа
        const auto found_docs = server.FindTopDocuments(std::string{ "cat dog duck good hill" });

        ASSERT_EQUAL(found_docs.size(), 5u);
        ASSERT_EQUAL(found_docs[0].id, doc_id3);
        ASSERT_EQUAL(found_docs[1].id, doc_id);
        ASSERT_EQUAL(found_docs[2].id, doc_id4);
        ASSERT_EQUAL(found_docs[3].id, doc_id2);
        ASSERT_EQUAL(found_docs[4].id, doc_id1);
    }

    {// проверка 6 документа
        server.AddDocument(doc_id5, std::string{ "my room is good anouth" }, DocumentStatus::ACTUAL, { 1 });
        const auto found_docs = server.FindTopDocuments(std::string{ "cat dog duck good hill" });

        ASSERT_EQUAL(found_docs.size(), 5u);
        ASSERT_EQUAL(found_docs[0].id, doc_id);
        ASSERT_EQUAL(found_docs[1].id, doc_id4);
        ASSERT_EQUAL(found_docs[2].id, doc_id3);
        ASSERT_EQUAL(found_docs[3].id, doc_id2);
        ASSERT_EQUAL(found_docs[4].id, doc_id1);
    }
}

void Test_CalculateRating() {
    const std::string content = std::string{ "cat in cool city" };
    const int arithmetic_mean_document_1 = 2;
    const int arithmetic_mean_document_2 = 5;
    const int arithmetic_mean_document_3 = -3;
    const int arithmetic_mean_document_4 = 1;
    SearchServer server(std::string{ "" });
    server.AddDocument(1, content, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, content, DocumentStatus::ACTUAL, { 4, 28, -16 });
    server.AddDocument(3, content, DocumentStatus::ACTUAL, { 4, -16, 3 });
    server.AddDocument(4, content, DocumentStatus::ACTUAL, { 1, 3, 1 });
    auto found_docs = server.FindTopDocuments(std::string{ "cat" });
    std::sort(std::execution::par, found_docs.begin(), found_docs.end(), [](const Document& lhs, const Document& rhs) {
        return lhs.id < rhs.id;
    });

    ASSERT_EQUAL(found_docs.size(), 4u);
    ASSERT_EQUAL(found_docs[0].rating, arithmetic_mean_document_1);
    ASSERT_EQUAL(found_docs[1].rating, arithmetic_mean_document_2);
    ASSERT_EQUAL(found_docs[2].rating, arithmetic_mean_document_3);
    ASSERT_EQUAL(found_docs[3].rating, arithmetic_mean_document_4);
}

void Test_FindTopDocuments_WithStatus_ResultFind(DocumentStatus status, int etalonId)
{
    const int doc_id = 42;
    const int doc_id1 = 104;
    const int doc_id2 = 950;
    const int doc_id3 = 272;
    const std::string content = std::string{ "cat in cool city" };
    const std::vector ratings = { 1 };
    SearchServer server(std::string{ "" });
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id1, content, DocumentStatus::REMOVED, ratings);
    server.AddDocument(doc_id2, content, DocumentStatus::BANNED, ratings);
    server.AddDocument(doc_id3, content, DocumentStatus::IRRELEVANT, ratings);

    const auto found_docs = server.FindTopDocuments(std::string{ "cat" }, status);

    ASSERT_EQUAL(found_docs.size(), 1u);
    ASSERT_EQUAL(found_docs[0].id, etalonId);
}

void Test_RelevanceCalculation() {
    SearchServer server(std::string{ "" });
    const double relevance_doc_1_and_2 = 0.135155;
    const double relevance_doc_3 = 0.366204;
    server.AddDocument(1, std::string{ "cat and dog" }, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(2, std::string{ "cat and cow" }, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(3, std::string{ "city" }, DocumentStatus::ACTUAL, { 1 });

    {
        auto foundDocs = server.FindTopDocuments(std::string{ "cat" });
        std::sort(std::execution::par, foundDocs.begin(), foundDocs.end(),
            [](const Document& lhs, const Document& rhs) {
            return lhs.id < rhs.id;
        });

        ASSERT_EQUAL(foundDocs.size(), 2u);
        ASSERT_EQUAL(foundDocs[0].relevance, relevance_doc_1_and_2);  // частный случай метода, когда входные данные типа double
        ASSERT_EQUAL(foundDocs[1].relevance, relevance_doc_1_and_2);
    }
    {
        auto foundDocs = server.FindTopDocuments(std::string{ "dog" });

        ASSERT_EQUAL(foundDocs.size(), 1u);
        ASSERT_EQUAL(foundDocs[0].relevance, relevance_doc_3);
    }
}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(Test_ExcludeStopWords_FromAddedDocumentContent);
    RUN_TEST(AddDocumentTest);
    RUN_TEST(TestExcludedMinusWords_WithOneDocument_ResultEmpty);
    RUN_TEST_WITH_ARG(Test_ExcludedMinusWords_WithOneDocument_ResultFindDocument(std::string{ "cat" }));
    RUN_TEST_WITH_ARG(Test_ExcludedMinusWords_WithOneDocument_ResultFindDocument(std::string{ "cat -dog" }));
    RUN_TEST_WITH_ARG(TestExcludedMinusWords_FromTwoDocument_ResultOneDocument(std::string{ "in -world" }, 42));
    RUN_TEST_WITH_ARG(TestExcludedMinusWords_FromTwoDocument_ResultOneDocument(std::string{ "in -city" }, 104));
    RUN_TEST(TestExcludedMinusWords_FromTwoDocuments_ResultTwoDocuments);
    RUN_TEST(TestExcludedMinusWords_FromTwoDocuments_ResultEmpty);
    RUN_TEST(Test_GetDocumentCount_ResultEmpty);
    RUN_TEST(Test_GetDocumentCount_ResultThreeDocuments);
    RUN_TEST(Test_GetDocumentId);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(Test_FindTopDocuments_SortByRelevance);
    RUN_TEST(Test_CalculateRating);
    RUN_TEST_WITH_ARG(Test_FindTopDocuments_WithStatus_ResultFind(DocumentStatus::ACTUAL, 42));
    RUN_TEST_WITH_ARG(Test_FindTopDocuments_WithStatus_ResultFind(DocumentStatus::BANNED, 950));
    RUN_TEST_WITH_ARG(Test_FindTopDocuments_WithStatus_ResultFind(DocumentStatus::IRRELEVANT, 272));
    RUN_TEST_WITH_ARG(Test_FindTopDocuments_WithStatus_ResultFind(DocumentStatus::REMOVED, 104));
    RUN_TEST_WITH_ARG(Test_FindTopDocuments_WithPredicat_EmptySearchServer_ResultEmpty([](int id, DocumentStatus status, int rating) {return rating > 5; }, std::string{ "cat dog" }));
    RUN_TEST_WITH_ARG(Test_FindTopDocuments_WithPredicat_EmptySearchServer_ResultEmpty([](int id, DocumentStatus status, int rating) {return rating > 5; }, std::string{ "" }));
    RUN_TEST_WITH_ARG(Test_FindTopDocuments_WithPredicat_ResultEmpty([](int id, DocumentStatus status, int rating) {return rating > 5; }));
    RUN_TEST_WITH_ARG(Test_FindTopDocuments_WithPredicat_ResultEmpty([](int id, DocumentStatus status, int rating) {return status != DocumentStatus::ACTUAL; }));
    RUN_TEST_WITH_ARG(Test_FindTopDocuments_WithPredicat_ResultEmpty([](int id, DocumentStatus status, int rating) {return id != 42; }));
    RUN_TEST_WITH_ARG(Test_FindTopDocuments_WithPredicat([](int id, DocumentStatus status, int rating) {return rating <= 5; }));
    RUN_TEST_WITH_ARG(Test_FindTopDocuments_WithPredicat([](int id, DocumentStatus status, int rating) {return status == DocumentStatus::ACTUAL; }));
    RUN_TEST_WITH_ARG(Test_FindTopDocuments_WithPredicat([](int id, DocumentStatus status, int rating) {return id == 42;  }));
    RUN_TEST(Test_RelevanceCalculation);
}