#include "remove_duplicates.h"
#include <set>
#include <vector>
#include <iterator>

void RemoveDuplicates(SearchServer& search_server) {
	std::map<std::set<std::string>, int> unic_documents;
	std::vector<int> delete_id;

	for (int id : search_server) {
		std::set<std::string> document_checked_uniqueness;

		auto str_freq = search_server.GetWordFrequencies(id);

		std::transform(str_freq.begin(), str_freq.end(), std::inserter(document_checked_uniqueness, document_checked_uniqueness.begin()), [](std::pair<const std::string, double>& T1) { return T1.first; });

		if (auto [iter, emplace_done] = unic_documents.emplace(document_checked_uniqueness, id); !emplace_done) {
			if (iter->second > id) {
				delete_id.push_back(iter->second);
				unic_documents.erase(iter);
				unic_documents[document_checked_uniqueness] = id;
			}
			else {
				delete_id.push_back(id);
			}
		}
	}

	for (int i : delete_id) {
		std::cout << "Found duplicate document id " << i << std::endl;
		search_server.RemoveDocument(i);
	}
}