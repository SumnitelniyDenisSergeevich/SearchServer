#include "search_server_tests.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "request_queue.h"

int main() {
    TestSearchServer();

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
}
