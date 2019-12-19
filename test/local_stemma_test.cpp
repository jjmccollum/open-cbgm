/*
 * local_stemma_test.cpp
 *
 *  Created on: Oct 31, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>
#include <set>
#include <map>

#include "pugixml.h"
#include "roaring.hh"
#include "local_stemma.h"

using namespace std;

void test_local_stemma_no_collapsing() {
	cout << "Running test_local_stemma_no_collapsing..." << endl;
	//Parse the test XML file:
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("examples/3_john_collation_fully_connected.xml");
	cout << "XML file load result: " << result.description() << endl;
	pugi::xml_node app_node = doc.select_node("descendant::app[@n=\"B25K1V10U32-38\"]").node();
	string label = app_node.child("label").text().get();
	pugi::xml_node graph_node = app_node.child("graph");
	local_stemma ls = local_stemma(label, graph_node, map<string, string>());
	cout << "label: " << ls.get_label() << endl;
	cout << "graph: " << endl;
	for (local_stemma_vertex v : ls.get_graph().vertices) {
		cout << v.id << endl;
	}
	for (local_stemma_edge e : ls.get_graph().edges) {
		cout << "(" << e.prior << ", " << e.posterior << ")" << endl;
	}
	cout << "closure_set: " << endl;
	for (pair<string, string> edge : ls.get_closure_set()) {
		cout << "(" << edge.first << ", " << edge.second << ")" << endl;
	}
	cout << "Done." << endl;
	return;
}

void test_local_stemma_with_collapsing() {
	cout << "Running test_local_stemma_with_collapsing..." << endl;
	//Parse the test XML file:
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("examples/3_john_collation_fully_connected.xml");
	cout << "XML file load result: " << result.description() << endl;
	pugi::xml_node app_node = doc.select_node("descendant::app[@n=\"B25K1V10U32-38\"]").node();
	string label = app_node.child("label").text().get();
	pugi::xml_node graph_node = app_node.child("graph");
	local_stemma ls = local_stemma(label, graph_node, map<string, string>({{"af1", "a"}, {"af2", "a"}, {"af3", "a"}}));
	cout << "label: " << ls.get_label() << endl;
	cout << "graph: " << endl;
	for (local_stemma_vertex v : ls.get_graph().vertices) {
		cout << v.id << endl;
	}
	for (local_stemma_edge e : ls.get_graph().edges) {
		cout << "(" << e.prior << ", " << e.posterior << ")" << endl;
	}
	cout << "closure_set: " << endl;
	for (pair<string, string> edge : ls.get_closure_set()) {
		cout << "(" << edge.first << ", " << edge.second << ")" << endl;
	}
	cout << "Done." << endl;
	return;
}

void test_is_equal_or_prior() {
	cout << "Running test_is_equal_or_prior..." << endl;
	//Parse the test XML file:
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("examples/3_john_collation_fully_connected.xml");
	cout << "XML file load result: " << result.description() << endl;
	pugi::xml_node app_node = doc.select_node("descendant::app[@n=\"B25K1V10U32-38\"]").node();
	string label = app_node.child("label").text().get();
	pugi::xml_node graph_node = app_node.child("graph");
	local_stemma ls = local_stemma(label, graph_node, map<string, string>({{"af1", "a"}, {"af2", "a"}, {"af3", "a"}}));
	//Test equality:
	if (!ls.is_equal_or_prior("a", "a")) {
		cout << "Error: is_equal_or_prior(\"a\", \"a\") should return true." << endl;
	}
	//Test direct priority:
	if (!ls.is_equal_or_prior("a", "b")) {
		cout << "Error: is_equal_or_prior(\"a\", \"b\") should return true." << endl;
	}
	//Test transitive priority:
	if (!ls.is_equal_or_prior("a", "b")) {
		cout << "Error: is_equal_or_prior(\"a\", \"b\") should return true." << endl;
	}
	//Test posteriority:
	if (ls.is_equal_or_prior("c", "a")) {
		cout << "Error: is_equal_or_prior(\"c\", \"a\") should return false." << endl;
	}
	//Test unrelated:
	if (ls.is_equal_or_prior("b", "c")) {
		cout << "Error: is_equal_or_prior(\"b\", \"c\") should return false." << endl;
	}
	cout << "Done." << endl;
	return;
}

void test_to_dot() {
	cout << "Running test_to_dot..." << endl;
	//Parse the test XML file:
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("examples/3_john_collation_fully_connected.xml");
	cout << "XML file load result: " << result.description() << endl;
	pugi::xml_node app_node = doc.select_node("descendant::app[@n=\"B25K1V10U32-38\"]").node();
	string label = app_node.child("label").text().get();
	pugi::xml_node graph_node = app_node.child("graph");
	local_stemma ls = local_stemma(label, graph_node, map<string, string>({{"af1", "a"}, {"af2", "a"}, {"af3", "a"}}));
	//Test .dot output on command-line output:
	ls.to_dot(cout);
	cout << "Done." << endl;
	return;
}

int main() {
	/**
	 * Entry point to the script. Calls all test methods.
	 */
	test_local_stemma_no_collapsing();
	test_local_stemma_with_collapsing();
	test_is_equal_or_prior();
	test_to_dot();
}
