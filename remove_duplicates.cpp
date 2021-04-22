#include "remove_duplicates.h"
#include <set>
#include <vector>

void RemoveDuplicates(SearchServer& search_server) {
	std::set< std::pair< std::set<std::string>, int > > unic_documents;
	std::set<std::string> unic_chek;
	bool b;
	std::vector<int> delete_id;

	for (int id : search_server) {
		unic_chek.clear();
		b = true;

		auto str_freq = search_server.GetWordFrequencies(id);

		for (auto& [str, freq] : str_freq) {
			unic_chek.insert(str);
		}

		for (auto& [str, id_doc] : unic_documents) {
			if (str == unic_chek) {

				if (id_doc > id) {
					delete_id.push_back(id_doc);
					unic_documents.erase(std::pair{ str, id_doc });
					unic_documents.insert(std::pair{ str, id });
				}
				else {
					delete_id.push_back(id);
				}

				b = false;
				break;
			}
		}

		if (b) {
			unic_documents.insert(std::pair{ unic_chek, id });
		}
	}

	for (int i : delete_id) {
		std::cout << "Found duplicate document id " << i << std::endl;
		search_server.RemoveDocument(i);
	}
}