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

//Define graph types for the stemma:
struct global_stemma_vertex {
	std::string id;
};
struct global_stemma_edge {
	std::string ancestor;
	std::string descendant;
	float length;
	float strength;
};

class global_stemma {
private:
	std::list<global_stemma_vertex> vertices;
	std::list<global_stemma_edge> edges;
public:
	global_stemma();
	global_stemma(const std::list<witness> & witnesses);
	virtual ~global_stemma();
	std::list<global_stemma_vertex> get_vertices() const;
	std::list<global_stemma_edge> get_edges() const;
	void to_dot(std::ostream & out, bool print_lengths=false, bool flow_strengths=false);
	void to_json(std::ostream & out);
};

#endif /* GLOBAL_STEMMA_H */
