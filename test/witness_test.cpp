/*
 * apparatus_test.cpp
 *
 *  Created on: Nov 8, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>
#include <set>

#include "pugixml.h"
#include "apparatus.h"
#include "witness.h"

using namespace std;

void test_witness() {
	cout << "Running test_witness..." << endl;
	//Parse the test XML file:
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("examples/acts_1_collation.xml");
	cout << "XML file load result: " << result.description() << endl;
	pugi::xml_node tei_node = doc.child("TEI");
	set<string> substantive_reading_types = set<string>({"substantive", "split"});
	apparatus app = apparatus(tei_node, substantive_reading_types);
	//Now initialize the witnesses using this apparatus:
	witness mt = witness("MT", app);
	cout << "id: " << mt.get_id() << endl;
	cout << "agreements with 33: " << mt.get_agreements_for_witness("33").toString() << endl;
	cout << "readings explained by 33: " << mt.get_explained_readings_for_witness("33").toString() << endl;
	witness ga_33 = witness("33", app);
	cout << "id: " << ga_33.get_id() << endl;
	cout << "agreements with MT: " << ga_33.get_agreements_for_witness("MT").toString() << endl;
	cout << "readings explained by MT: " << ga_33.get_explained_readings_for_witness("MT").toString() << endl;
	cout << "Done." << endl;
	return;
}

int main() {
	/**
	 * Entry point to the script. Calls all test methods.
	 */
	test_witness();
}
