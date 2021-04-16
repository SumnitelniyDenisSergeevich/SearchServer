#include "search_server_tests.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "request_queue.h"
#include "log_duration.h"

using namespace std::literals;

int main() {
    TestSearchServer();
    setlocale(LC_ALL, "Russian");
    SearchServer search_server("и в на"s);

    AddDocument(search_server, 1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    AddDocument(search_server, 1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
    AddDocument(search_server, -1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
    AddDocument(search_server, 3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    AddDocument(search_server, 4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    std::cout << std::endl;
    {
        LOG_DURATION_STREAM("Operation time"s, std::cout);
        FindTopDocuments(search_server, "пушистый -пёс"s);
    }
    std::cout << std::endl;
    {
        LOG_DURATION_STREAM("Operation time"s, std::cout);
        FindTopDocuments(search_server, "пушистый --кот"s);
    }
    std::cout << std::endl;
    {
        LOG_DURATION_STREAM("Operation time"s, std::cout);
        FindTopDocuments(search_server, "пушистый -"s);
    }
    std::cout << std::endl;

    {
        LOG_DURATION_STREAM("Operation time"s, std::cout);
        MatchDocuments(search_server, "пушистый пёс"s);
    }
    std::cout << std::endl;
    {
        LOG_DURATION_STREAM("Operation time"s, std::cout);
        MatchDocuments(search_server, "модный -кот"s);
    }
    std::cout << std::endl;
    {
        LOG_DURATION_STREAM("Operation time"s, std::cout);
        MatchDocuments(search_server, "модный --пёс"s);
    }
    std::cout << std::endl;
    {
        LOG_DURATION_STREAM("Operation time"s, std::cout);
        MatchDocuments(search_server, "пушистый - хвост"s);
    }

    system("pause");
}