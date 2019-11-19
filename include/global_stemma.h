/*
 * global_stemma.h
 *
 *  Created on: Nov 14, 2019
 *      Author: jjmccollum
 */
#ifndef GLOBAL_STEMMA_H
#define GLOBAL_STEMMA_H

#include <iostream>
#include <string>
#include <unordered_map>

#include "apparatus.h"
#include "witness.h"

using namespace std;

//Define graph types for the stemma:
struct global_stemma_vertex {
	string id;
};
struct global_stemma_edge {
	string ancestor;
	string descendant;
	float weight;
	bool ambiguous;
};
struct global_stemma_graph {
	list<global_stemma_vertex> vertices;
	list<global_stemma_edge> edges;
};

class global_stemma {
private:
	global_stemma_graph graph;
public:
	global_stemma();
	global_stemma(apparatus & app, unordered_map<string, witness> & witnesses_by_id);
	virtual ~global_stemma();
	global_stemma_graph get_graph();
	void to_dot(std::ostream & out);
};

#endif /* GLOBAL_STEMMA_H */
