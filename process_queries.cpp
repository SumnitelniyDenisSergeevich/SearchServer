#include "process_queries.h"
#include <algorithm>

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) 
{
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
        [&search_server](auto querie) {return search_server.FindTopDocuments(querie); });
    return result;
}

std::list<Document> ProcessQueriesJoined(const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    return std::transform_reduce(std::execution::par,
        queries.begin(),
        queries.end(),
        std::list<Document>{},
        [](std::list<Document> ins_to, std::list<Document> ins_from) {
        ins_to.splice(ins_to.end(), std::move(ins_from));
        return ins_to; },
        [&search_server]( std::string_view raw_query) {
            std::vector<Document> vector_tmp = std::move(search_server.FindTopDocuments(raw_query));
            return std::list<Document>{ vector_tmp.begin(), vector_tmp.end() };
        });
}