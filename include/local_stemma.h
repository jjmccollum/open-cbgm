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
#include <set>
#include <map>

#include "pugixml.h"

using namespace std;

//Define graph types for the stemma:
struct local_stemma_vertex {
	string id;
};
struct local_stemma_edge {
	string prior;
	string posterior;
	float weight;
};
struct local_stemma_path {
	string prior;
	string posterior;
	float weight; //total weight of shortest path
	int cardinality; //number of edges with weight > 0 on path
};

class local_stemma {
private:
	string id;
	string label;
	list<local_stemma_vertex> vertices;
	list<local_stemma_edge> edges;
	list<string> roots;
	map<pair<string, string>, local_stemma_path> paths;
public:
	local_stemma();
	local_stemma(const pugi::xml_node & xml, const string & vu_id, const string & vu_label, const set<pair<string, string>> & split_pairs, const set<string> & trivial_readings, const set<string> & dropped_readings);
	local_stemma(const string & _id, const string & _label, const list<local_stemma_vertex> & _vertices, const list<local_stemma_edge> & _edges);
	virtual ~local_stemma();
	string get_id() const;
	string get_label() const;
	list<local_stemma_vertex> get_vertices() const;
	list<local_stemma_edge> get_edges() const;
	list<string> get_roots() const;
	map<pair<string, string>, local_stemma_path> get_paths() const;
	bool path_exists(const string & r1, const string & r2) const;
	local_stemma_path get_path(const string & r1, const string & r2) const;
	bool common_ancestor_exists(const string & r1, const string & r2) const;
	void to_dot(ostream & out, bool print_weights);
};

#endif /* LOCAL_STEMMA_H */
