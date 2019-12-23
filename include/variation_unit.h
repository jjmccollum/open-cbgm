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
#include <set>
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
	string id;
	string label;
	list<string> readings;
	unordered_map<string, list<string>> reading_support;
	int connectivity = 10;
	local_stemma stemma;
	textual_flow_graph graph;
public:
	variation_unit();
	variation_unit(const pugi::xml_node & xml, const set<string> & substantive_reading_types);
	virtual ~variation_unit();
	string get_id() const;
	string get_label() const;
	list<string> get_readings() const;
	unordered_map<string, list<string>> get_reading_support() const;
	int get_connectivity() const;
	local_stemma get_local_stemma() const;
	textual_flow_graph get_graph() const;
	void calculate_textual_flow_for_witness(const witness & w);
	void calculate_textual_flow(const list<witness> & witnesses);
	void textual_flow_diagram_to_dot(ostream & out);
	void textual_flow_diagram_for_reading_to_dot(const string & rdg_id, ostream & out);
	void textual_flow_diagram_for_changes_to_dot(ostream & out);
};

#endif /* VARIATION_UNIT_H */
