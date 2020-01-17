/*
 * textual_flow.h
 *
 *  Created on: Dec 24, 2019
 *      Author: jjmccollum
 */
#ifndef TEXTUAL_FLOW_H
#define TEXTUAL_FLOW_H

#include <iostream>
#include <string>
#include <list>

#include "pugixml.h"
#include "variation_unit.h"
#include "witness.h"

using namespace std;

enum flow_type {NONE, EQUAL, CHANGE, LOSS};

//Define graph types for the textual flow diagram:
struct textual_flow_vertex {
	string id;
	string rdg;
};
struct textual_flow_edge {
	string ancestor;
	string descendant;
	flow_type type;
	int connectivity;
	float strength;
};
struct textual_flow_graph {
	list<textual_flow_vertex> vertices;
	list<textual_flow_edge> edges;
};

class textual_flow {
private:
	string label;
	list<string> readings;
	int connectivity;
	textual_flow_graph graph;
public:
	textual_flow();
	textual_flow(const variation_unit & vu, const list<witness> & witnesses);
	virtual ~textual_flow();
	string get_label() const;
	list<string> get_readings() const;
	int get_connectivity() const;
	textual_flow_graph get_graph() const;
	void textual_flow_to_dot(ostream & out, bool flow_strengths);
	void coherence_in_attestations_to_dot(ostream & out, const string & rdg, bool flow_strengths);
	void coherence_in_variant_passages_to_dot(ostream & out, bool flow_strengths);
};

#endif /* TEXTUAL_FLOW_H */
