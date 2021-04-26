#include "remove_duplicates.h"
#include <set>
#include <vector>
#include <iterator>

void RemoveDuplicates(SearchServer& search_server) {
	std::map<std::set<std::string>, int> unic_documents;
	std::set<std::string> document_checked_uniqueness;
	bool document_is_unique;
	std::vector<int> delete_id;

	for (int id : search_server) {
		document_checked_uniqueness.clear();
		document_is_unique = true;

		auto str_freq = search_server.GetWordFrequencies(id);

		std::transform(str_freq.begin(), str_freq.end(), std::inserter(document_checked_uniqueness, document_checked_uniqueness.begin()), [](std::pair<const std::string, double>& T1) { return T1.first; });

		if (unic_documents.find(document_checked_uniqueness) != unic_documents.end()) {
			int doc_id = unic_documents.at(document_checked_uniqueness);
			if (doc_id > id) {
				delete_id.push_back(doc_id);
				unic_documents.erase(document_checked_uniqueness);
				unic_documents[document_checked_uniqueness] = id;
			}
			else {
				delete_id.push_back(id);
			}

			document_is_unique = false;
		}

		if (document_is_unique) {
			unic_documents.insert(std::pair{ document_checked_uniqueness, id });
		}
	}

	for (int i : delete_id) {
		std::cout << "Found duplicate document id " << i << std::endl;
		search_server.RemoveDocument(i);
	}
}