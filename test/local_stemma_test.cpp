/*
 * local_stemma_test.cpp
 *
 *  Created on: Oct 31, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>

#include "pugixml.h"
#include "roaring.hh"
#include "local_stemma.h"

using namespace std;

void test_local_stemma() {
	cout << "Running test_local_stemma..." << endl;
	//Parse the test XML file:
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("test/acts_1_collation.xml");
	cout << "XML file load result: " << result.description() << endl;
	pugi::xml_node label_node = doc.select_node("descendant::app/label[text()=\"Acts 1:13/30-38\"]").node();
	string label = label_node.text().get();
	pugi::xml_node graph_node = label_node.select_node("following-sibling::graph").node();
	local_stemma ls = local_stemma(label, graph_node);
	cout << "label: " << ls.get_label() << endl;
	cout << "graph: " << endl;
	for (local_stemma_vertex v : ls.get_graph().vertices) {
		cout << v.index << ": " << v.id << endl;
	}
	for (local_stemma_edge e : ls.get_graph().edges) {
		cout << "(" << e.from << ", " << e.to << ")" << endl;
	}
	cout << "closure_matrix: " << endl;
	for (Roaring row : ls.get_closure_matrix()) {
		cout << row.toString() << endl;
	}
	cout << "Done." << endl;
	return;
}

void test_is_equal_or_prior() {
	cout << "Running test_is_equal_or_prior..." << endl;
	//Parse the test XML file:
	pugi::xml_document doc;
	doc.load_file("test/acts_1_collation.xml");
	pugi::xml_node label_node = doc.select_node("//app/label[text()=\"Acts 1:13/30-38\"]").node();
	string label = label_node.text().get();
	pugi::xml_node graph_node = label_node.select_node("following-sibling::graph").node();
	local_stemma ls = local_stemma(label, graph_node);
	//Test equality:
	if (!ls.is_equal_or_prior(0, 0)) {
		cout << "Error: is_equal_or_prior(0, 0) should return true." << endl;
	}
	//Test direct priority:
	if (!ls.is_equal_or_prior(11, 1)) {
		cout << "Error: is_equal_or_prior(11, 1) should return true." << endl;
	}
	//Test transitive priority:
	if (!ls.is_equal_or_prior(0, 3)) {
		cout << "Error: is_equal_or_prior(0, 3) should return true." << endl;
	}
	//Test posteriority:
	if (ls.is_equal_or_prior(3, 0)) {
		cout << "Error: is_equal_or_prior(3, 0) should return false." << endl;
	}
	//Test unrelated:
	if (ls.is_equal_or_prior(0, 1)) {
		cout << "Error: is_equal_or_prior(0, 1) should return false." << endl;
	}
	cout << "Done." << endl;
	return;
}

void test_to_dot() {
	cout << "Running test_to_dot..." << endl;
	//Parse the test XML file:
	pugi::xml_document doc;
	doc.load_file("test/acts_1_collation.xml");
	pugi::xml_node label_node = doc.select_node("//app/label[text()=\"Acts 1:13/30-38\"]").node();
	string label = label_node.text().get();
	pugi::xml_node graph_node = label_node.select_node("following-sibling::graph").node();
	local_stemma ls = local_stemma(label, graph_node);
	//Test .dot output on command-line output:
	ls.to_dot(cout);
	cout << "Done." << endl;
	return;
}

int main() {
	/**
	 * Entry point to the script. Calls all test methods.
	 */
	test_local_stemma();
	test_is_equal_or_prior();
	test_to_dot();
}
