#pragma once

#include "search_server.h"
#include "document.h"
#include "Tests_Frame.h"

#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <map>



// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void AddDocumentTest();

void Test_ExcludeStopWords_FromAddedDocumentContent();

void Test_ExcludedMinusWords_WithOneDocument_ResultFindDocument(const std::string& query);

void TestExcludedMinusWords_WithOneDocument_ResultEmpty();

void TestExcludedMinusWords_FromTwoDocuments_ResultEmpty();

void TestExcludedMinusWords_FromTwoDocument_ResultOneDocument(const std::string& query, int etalonID);

void TestExcludedMinusWords_FromTwoDocuments_ResultTwoDocuments();

void Test_GetDocumentCount_ResultEmpty();

void Test_GetDocumentCount_ResultThreeDocuments();

void Test_GetDocumentId();

void TestMatchDocument();

[[nodiscard]] SearchServer Сreating_Filled_Object(int id, int id1, int id2, int id3, int id4);

void Test_FindTopDocuments_SortByRelevance();

void Test_CalculateRating();

void Test_FindTopDocuments_WithStatus_ResultFind(DocumentStatus status, int etalonId);

template<typename Func>
void Test_FindTopDocuments_WithPredicat_EmptySearchServer_ResultEmpty(Func predicate, const std::string& query)
{
    SearchServer server(std::string{ "" });

    const auto found_docs = server.FindTopDocuments(std::string{ "cat dog" }, predicate);

    ASSERT(found_docs.empty());
}

template<typename Func>
void Test_FindTopDocuments_WithPredicat_ResultEmpty(Func predicate)
{
    const int doc_id1 = 42;
    SearchServer server(std::string{ "" });
    server.AddDocument(doc_id1, std::string{ "cat in cool city" }, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments(std::string{ "cat dog" }, predicate);

    ASSERT(found_docs.empty());
}

template<typename Func>
void Test_FindTopDocuments_WithPredicat(Func predicate)
{
    const int doc_id1 = 42;
    SearchServer server(std::string{ "" });
    server.AddDocument(doc_id1, std::string{ "cat in cool city" }, DocumentStatus::ACTUAL, { 1 });

    const auto found_docs = server.FindTopDocuments(std::string{ "cat dog" }, predicate);

    ASSERT_EQUAL(found_docs.size(), 1u);
    ASSERT_EQUAL(found_docs[0].id, doc_id1);
}

void Test_RelevanceCalculation();


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();


