#include "search_server_tests.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "request_queue.h"

int main() {
    TestSearchServer();
  
    SearchServer search_server(std::string{ "and in at" });
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, std::string{ "curly cat curly tail" }, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, std::string{ "curly dog and fancy collar" }, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, std::string{ "big cat fancy collar " }, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, std::string{ "big dog sparrow Eugene" }, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, std::string{ "big dog sparrow Vasiliy" }, DocumentStatus::ACTUAL, { 1, 1, 1 });

    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest(std::string{ "empty request" });
    }
    request_queue.AddFindRequest(std::string{ "curly dog" });
    request_queue.AddFindRequest(std::string{ "big collar" });
    request_queue.AddFindRequest(std::string{ "sparrow" });
    std::cout << std::string{ "Total empty requests: " } << request_queue.GetNoResultRequests() << std::endl;
    system("pause");
    return 0;
}
