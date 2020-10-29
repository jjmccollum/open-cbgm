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

#include "pugixml.hpp"

//Define graph types for the stemma:
struct local_stemma_vertex {
	std::string id;
};
struct local_stemma_edge {
	std::string prior;
	std::string posterior;
	float weight;
};
struct local_stemma_path {
	std::string prior;
	std::string posterior;
	float weight; //total weight of shortest path
	int cardinality; //number of edges with weight > 0 on path
};

class local_stemma {
private:
	std::string id;
	std::string label;
	std::list<local_stemma_vertex> vertices;
	std::list<local_stemma_edge> edges;
	std::list<std::string> roots;
	std::map<std::pair<std::string, std::string>, local_stemma_path> paths;
public:
	local_stemma();
	local_stemma(const pugi::xml_node & xml, const std::string & vu_id, const std::string & vu_label, const std::set<std::pair<std::string, std::string>> & split_pairs, const std::set<std::string> & trivial_readings, const std::set<std::string> & dropped_readings);
	local_stemma(const std::string & _id, const std::string & _label, const std::list<local_stemma_vertex> & _vertices, const std::list<local_stemma_edge> & _edges);
	virtual ~local_stemma();
	std::string get_id() const;
	std::string get_label() const;
	std::list<local_stemma_vertex> get_vertices() const;
	std::list<local_stemma_edge> get_edges() const;
	std::list<std::string> get_roots() const;
	std::map<std::pair<std::string, std::string>, local_stemma_path> get_paths() const;
	bool path_exists(const std::string & r1, const std::string & r2) const;
	local_stemma_path get_path(const std::string & r1, const std::string & r2) const;
	bool common_ancestor_exists(const std::string & r1, const std::string & r2) const;
	void to_dot(std::ostream & out, bool print_weights=false);
	void to_json(std::ostream & out);
};

#endif /* LOCAL_STEMMA_H */
