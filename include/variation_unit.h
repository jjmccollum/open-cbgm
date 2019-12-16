/*
 * variation_unit.h
 *
 *  Created on: Oct 24, 2019
 *      Author: jjmccollum
 */

#ifndef VARIATION_UNIT_H
#define VARIATION_UNIT_H

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "pugixml.h"
#include "local_stemma.h"


using namespace std;

enum flow_type {NONE, EQUAL, CHANGE, AMBIGUOUS, LOSS};

//Define graph types for the textual flow diagram:
struct textual_flow_vertex {
	string id;
	bool extant;
	bool ambiguous;
};
struct textual_flow_edge {
	string ancestor;
	string descendant;
	int connectivity;
	flow_type type;
};
struct textual_flow_graph {
	list<textual_flow_vertex> vertices;
	list<textual_flow_edge> edges;
};

//Include a forward declaration of the witness class here, to avoid circular dependency:
class witness;

class variation_unit {
private:
	unsigned int index=0;
	string id;
	string label;
	list<string> readings;
	unordered_map<string, list<string>> reading_support;
	int connectivity = 10;
	local_stemma stemma;
	textual_flow_graph graph;
public:
	variation_unit();
	variation_unit(unsigned int variation_unit_index, const pugi::xml_node & xml, const unordered_set<string> & substantive_reading_types);
	virtual ~variation_unit();
	unsigned int get_index();
	string get_id();
	string get_label();
	list<string> get_readings();
	int size();
	unordered_map<string, list<string>> get_reading_support();
	int get_connectivity();
	local_stemma get_local_stemma();
	textual_flow_graph get_textual_flow_diagram();
	void calculate_textual_flow_for_witness(const witness & w);
	void calculate_textual_flow(const unordered_map<string, witness> & witnesses_by_id);
	void textual_flow_diagram_to_dot(ostream & out);
	void textual_flow_diagram_for_reading_to_dot(const string & rdg_id, ostream & out);
	void textual_flow_diagram_for_changes_to_dot(ostream & out);
};

#endif /* VARIATION_UNIT_H */
