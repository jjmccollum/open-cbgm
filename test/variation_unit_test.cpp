/*
 * local_stemma_test.cpp
 *
 *  Created on: Nov 4, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <list>

#include "pugixml.h"
#include "variation_unit.h"

using namespace std;

void print_reading_support(unordered_map<string, list<string>> reading_support) {
	cout << "{ ";
	for (pair<string, list<string>> kv : reading_support) {
		cout << "\"" << kv.first << "\": ";
		cout << "[ ";
		for (string rdg_id : kv.second) {
			cout << rdg_id << " ";
		}
		cout << "] ";
	}
	cout << "} " << endl;
}

void test_variation_unit() {
	cout << "Running test_variation_unit..." << endl;
	//Parse the test XML file:
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("test/acts_1_collation.xml");
	cout << "XML file load result: " << result.description() << endl;
	pugi::xml_node app_node = doc.select_node("descendant::app/label[text()=\"Acts 1:13/30-38\"]").node().parent();
	unordered_set<string> distinct_reading_types = unordered_set<string>({"substantive"});
	variation_unit vu = variation_unit(0, app_node, distinct_reading_types);
	cout << "label: " << vu.get_label() << endl;
	cout << "reading_support, substantive only: " << endl;
	unordered_map<string, list<string>> reading_support = vu.get_reading_support();
	print_reading_support(reading_support);
	cout << "reading_support, substantive and split: " << endl;
	distinct_reading_types = unordered_set<string>({"substantive", "split"});
	vu = variation_unit(0, app_node, distinct_reading_types);
	reading_support = vu.get_reading_support();
	print_reading_support(reading_support);
	cout << "reading_support, substantive, split, and orthographic: " << endl;
	distinct_reading_types = unordered_set<string>({"substantive", "split", "orthographic"});
	vu = variation_unit(0, app_node, distinct_reading_types);
	reading_support = vu.get_reading_support();
	print_reading_support(reading_support);
	cout << "reading_support, substantive, split, orthographic, and defective: " << endl;
	distinct_reading_types = unordered_set<string>({"substantive", "split", "orthographic", "defective"});
	vu = variation_unit(0, app_node, distinct_reading_types);
	reading_support = vu.get_reading_support();
	print_reading_support(reading_support);
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
