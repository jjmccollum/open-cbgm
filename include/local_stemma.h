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
#include <list>
#include <vector>

#include "pugixml.h"
#include "roaring.hh"

using namespace std;

//Define graph types for the stemma:
struct local_stemma_vertex {
	int index;
	string id;
};
struct local_stemma_edge {
	int from;
	int to;
};
struct local_stemma_graph {
	list<local_stemma_vertex> vertices;
	list<local_stemma_edge> edges;
};

class local_stemma {
private:
	string label;
	local_stemma_graph graph;
	vector<Roaring> closure_matrix;
public:
	local_stemma();
	local_stemma(string label, const pugi::xml_node xml);
	virtual ~local_stemma();
	string get_label();
	local_stemma_graph get_graph();
	vector<Roaring> get_closure_matrix();
	bool is_equal_or_prior(int i, int j);
	void to_dot(std::ostream & out);
};

#endif /* LOCAL_STEMMA_H */
