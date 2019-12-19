/*
 * apparatus_test.cpp
 *
 *  Created on: Nov 8, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>
#include <list>
#include <set>

#include "pugixml.h"
#include "apparatus.h"
#include "witness.h"

using namespace std;

void test_witness() {
	cout << "Running test_witness..." << endl;
	//Parse the test XML file:
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("examples/3_john_collation_fully_connected.xml");
	cout << "XML file load result: " << result.description() << endl;
	pugi::xml_node tei_node = doc.child("TEI");
	set<string> distinct_reading_types = set<string>({"split"});
	apparatus app = apparatus(tei_node, distinct_reading_types);
	//Now initialize the witnesses using this apparatus:
	witness a = witness("A", app);
	cout << "id: " << a.get_id() << endl;
	cout << "extant readings: " << a.get_explained_readings_for_witness("A").toString() << endl;
	cout << "agreements with 2147: " << a.get_agreements_for_witness("2147").toString() << endl;
	cout << "readings explained by 2147: " << a.get_explained_readings_for_witness("2147").toString() << endl;
	cout << "readings not explained by 2147: " << (a.get_explained_readings_for_witness("A") ^ a.get_explained_readings_for_witness("2147")).toString() << endl;
	witness ga_2147 = witness("2147", app);
	cout << "id: " << ga_2147.get_id() << endl;
	cout << "extant readings: " << ga_2147.get_explained_readings_for_witness("2147").toString() << endl;
	cout << "agreements with A: " << ga_2147.get_agreements_for_witness("A").toString() << endl;
	cout << "readings explained by A: " << ga_2147.get_explained_readings_for_witness("A").toString() << endl;
	cout << "readings not explained by A: " << (ga_2147.get_explained_readings_for_witness("2147") ^ ga_2147.get_explained_readings_for_witness("A")).toString() << endl;
	cout << "Done." << endl;
	return;
}

int main() {
	/**
	 * Entry point to the script. Calls all test methods.
	 */
	test_witness();
}
