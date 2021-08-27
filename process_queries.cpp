#include "process_queries.h"
#include <algorithm>

using namespace std;

vector<vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const vector<string>& queries) 
{
    vector<vector<Document>> result(queries.size());
    transform(execution::par, queries.begin(), queries.end(), result.begin(),
        [&search_server](auto querie) {return search_server.FindTopDocuments(querie); });
    return result;
}

list<Document> ProcessQueriesJoined(const SearchServer& search_server,
    const vector<string>& queries)
{
    return transform_reduce(execution::par,
        queries.begin(),
        queries.end(),
        list<Document>{},
        [](list<Document> ins_to, list<Document> ins_from) {
        ins_to.splice(ins_to.end(), move(ins_from));
        return ins_to; },
        [&search_server]( string_view raw_query) {
            vector<Document> vector_tmp = move(search_server.FindTopDocuments(raw_query));
            return list<Document>{ vector_tmp.begin(), vector_tmp.end() };
        });
}