/*
 * local_stemma.h
 *
 *  Created on: Oct 31, 2019
 *      Author: jjmccollum
 */
#ifndef LOCAL_STEMMA_H
#define LOCAL_STEMMA_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <list>
#include <set>

#include "pugixml.h"

using namespace std;

//Define graph types for the stemma:
struct local_stemma_vertex {
	string id;
};
struct local_stemma_edge {
	string prior;
	string posterior;
};
struct local_stemma_graph {
	list<local_stemma_vertex> vertices;
	list<local_stemma_edge> edges;
};

class local_stemma {
private:
	string label;
	local_stemma_graph graph;
	set<pair<string, string>> closure_set;
public:
	local_stemma();
	local_stemma(string label, const pugi::xml_node xml, unordered_map<string, string> trivial_to_significant);
	virtual ~local_stemma();
	string get_label();
	local_stemma_graph get_graph();
	set<pair<string, string>> get_closure_set();
	bool is_equal_or_prior(string r1, string r2);
	void to_dot(std::ostream & out);
};

#endif /* LOCAL_STEMMA_H */
