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
#include <list>

#include "witness.h"

using namespace std;

//Define graph types for the stemma:
struct global_stemma_vertex {
	string id;
};
struct global_stemma_edge {
	string ancestor;
	string descendant;
	float length;
	float strength;
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
	global_stemma(const list<witness> & witnesses);
	virtual ~global_stemma();
	global_stemma_graph get_graph() const;
	void to_dot(ostream & out, bool print_lengths, bool flow_strengths);
};

#endif /* GLOBAL_STEMMA_H */
