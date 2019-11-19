/*
 * local_stemma_test.cpp
 *
 *  Created on: Nov 4, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>
#include <unordered_map>
#include <list>

#include "pugixml.h"
#include "variation_unit.h"

using namespace std;

void test_variation_unit() {
	cout << "Running test_variation_unit..." << endl;
	//Parse the test XML file:
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("test/acts_1_collation.xml");
	cout << "XML file load result: " << result.description() << endl;
	pugi::xml_node app_node = doc.select_node("descendant::app/label[text()=\"Acts 1:13/30-38\"]").node().parent();
	variation_unit vu = variation_unit(0, app_node);
	cout << "label: " << vu.get_label() << endl;
	cout << "reading_support: " << endl;
	unordered_map<string, list<int>> reading_support = vu.get_reading_support();
	cout << "{ ";
	for (pair<string, list<int>> kv : reading_support) {
		cout << "\"" << kv.first << "\": ";
		cout << "[ ";
		for (int reading_ind : kv.second) {
			cout << reading_ind << " ";
		}
		cout << "] ";
	}
	cout << "} " << endl;
	cout << "connectivity: " << vu.get_connectivity() << endl;
	cout << "Done." << endl;
	return;
}

int main() {
	/**
	 * Entry point to the script. Calls all test methods.
	 */
	test_variation_unit();
}
