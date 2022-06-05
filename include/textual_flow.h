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

#include "variation_unit.h"
#include "witness.h"

enum flow_type {NONE, EQUAL, CHANGE, LOSS};

//Define graph types for the textual flow diagram:
struct textual_flow_vertex {
	std::string id;
	std::string rdg;
};
struct textual_flow_edge {
	std::string ancestor;
	std::string descendant;
	flow_type type;
	int connectivity;
	float strength;
};

class textual_flow {
private:
	std::string label;
	std::list<std::string> readings;
	int connectivity;
	std::list<textual_flow_vertex> vertices;
	std::list<textual_flow_edge> edges;
public:
	textual_flow();
	textual_flow(const variation_unit & vu, const std::list<witness> & witnesses, int _connectivity);
	textual_flow(const variation_unit & vu, const std::list<witness> & witnesses);
	virtual ~textual_flow();
	std::string get_label() const;
	std::list<std::string> get_readings() const;
	int get_connectivity() const;
	std::list<textual_flow_vertex> get_vertices() const;
	std::list<textual_flow_edge> get_edges() const;
	void textual_flow_to_dot(std::ostream & out, bool flow_strengths=false);
	void textual_flow_to_json(std::ostream & out);
	void coherence_in_attestations_to_dot(std::ostream & out, const std::string & rdg, bool flow_strengths=false);
	void coherence_in_attestations_to_json(std::ostream & out, const std::string & rdg);
	void coherence_in_variant_passages_to_dot(std::ostream & out, bool flow_strengths=false);
	void coherence_in_variant_passages_to_json(std::ostream & out);
};

#endif /* TEXTUAL_FLOW_H */
