/*
 * local_stemma.cpp
 *
 *  Created on: Oct 31, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>
#include <list>
#include <set> //used instead of unordered_set because pair does not have a default hash function and readings are few enough for tree structures to be more efficient
#include <map> //used instead of unordered_map because readings are few enough for tree structures to be more efficient

#include "pugixml.h"
#include "local_stemma.h"

using namespace std;

/**
 * Recursive function for populating a transitive closure adjacency matrix given an adjacency list for a graph.
 */
void transitive_closure_dfs(const string & prior, const string & posterior, const map<string, list<string>> & adjacency_map, set<pair<string, string>> & closure_set) {
	closure_set.insert(pair<string, string>(prior, posterior));
	for (string next : adjacency_map.at(posterior)) {
		if (closure_set.find(pair<string, string>(prior, next)) == closure_set.end()) {
			transitive_closure_dfs(prior, next, adjacency_map, closure_set);
		}
	}
	return;
}

/**
 * Default constructor.
 */
local_stemma::local_stemma() {

}

/**
 * Constructs a local stemma from a <graph/> XML element using the given label taken from the apparatus.
 * A map of trivial reading IDs to their significant parent readings (which may optionally be empty) is also used to collapse the graph in the XML element.
 */
local_stemma::local_stemma(const string & apparatus_label, const pugi::xml_node & xml, const map<string, string> & trivial_to_significant) {
	graph.vertices = list<local_stemma_vertex>();
	graph.edges = list<local_stemma_edge>();
	//Set the label:
	label = apparatus_label;
	//Add a vertex for each <node/> element:
	for (pugi::xml_node node : xml.children("node")) {
		//Do not add any node that lacks an "n" attribute:
		if (!node.attribute("n")) {
			continue;
		}
		//Map node "n" attributes to their indices:
		string id = node.attribute("n").value();
		//Skip any readings with trivial IDs:
		if (trivial_to_significant.find(id) != trivial_to_significant.end()) {
			continue;
		}
		//Then create a vertex and add it to the graph:
		local_stemma_vertex v;
		v.id = id;
		graph.vertices.push_back(v);
	}
	//Add an edge for each <arc/> element:
	set<pair<string, string>> edge_pairs;
	for (pugi::xml_node arc : xml.children("arc")) {
		string prior = arc.attribute("from").value();
		string posterior = arc.attribute("to").value();
		//Map any trivial readings to their significant parent readings:
		if (trivial_to_significant.find(prior) != trivial_to_significant.end()) {
			prior = trivial_to_significant.at(prior);
		}
		if (trivial_to_significant.find(posterior) != trivial_to_significant.end()) {
			posterior = trivial_to_significant.at(posterior);
		}
		//If the resulting readings are identical, then do not add the collapsed edge:
		if (prior == posterior) {
			continue;
		}
		//Otherwise, add the collapsed edge to the set:
		edge_pairs.insert(pair<string, string>(prior, posterior));
	}
	//Then add each edge to the graph:
	for (pair<string, string> edge_pair : edge_pairs) {
		local_stemma_edge e;
		e.prior = edge_pair.first;
		e.posterior = edge_pair.second;
		graph.edges.push_back(e);
	}
	//Construct an adjacency map from the graph:
	map<string, list<string>> adjacency_map = map<string, list<string>>();
	for (local_stemma_vertex v: graph.vertices) {
		adjacency_map[v.id] = list<string>();
	}
	for (local_stemma_edge e : graph.edges) {
		adjacency_map[e.prior].push_back(e.posterior);
	}
	//Now use depth-first traversal to populate the adjacency matrix of the transitive closure:
	closure_set = set<pair<string, string>>();
	for (local_stemma_vertex v : graph.vertices) {
		string id = v.id;
		//Recursively mark all vertices reachable from this one, including this vertex itself:
		transitive_closure_dfs(id, id, adjacency_map, closure_set);
	}
}

/**
 * Default destructor.
 */
local_stemma::~local_stemma() {

}

/**
 * Returns the label for this local_stemma.
 */
string local_stemma::get_label() const {
	return label;
}

/**
 * Returns the graph structure for this local_stemma.
 */
local_stemma_graph local_stemma::get_graph() const {
	return graph;
}

/**
 * Return the set of edges in the transitive closure of this local_stemma's graph.
 */
set<pair<string, string>> local_stemma::get_closure_set() const {
	return closure_set;
}

/**
 * Given two reading indices, checks if they are equal
 * or if the first reading is prior to the second
 * (i.e., if the first reading is adjacent to the second in the transitive closure of the stemma graph).
 */
bool local_stemma::is_equal_or_prior(const string & r1, const string & r2) const {
	return closure_set.find(pair<string, string>(r1, r2)) != closure_set.end();
}

/**
 * Given an output stream, writes the local stemma graph to output in .dot format.
 */
void local_stemma::to_dot(ostream & out) {
	//Add the graph first:
	out << "digraph local_stemma {\n";
	//Add a line indicating that nodes do not have any shape:
	out << "\tnode [shape=plaintext];\n";
	//Add a box node indicating the label of this variation_unit:
	out << "\tlabel [shape=box, label=\"" << label << "\"];\n";
	//Add all of its nodes, assigning them numerical indices:
	map<string, int> id_to_index = map<string, int>();
	for (local_stemma_vertex v : graph.vertices) {
		//Add the vertex's ID to the map:
		id_to_index[v.id] = id_to_index.size();
		out << "\t";
		out << id_to_index.at(v.id);
		//Use the ID as the node label:
		out << " [label=\"" << v.id << "\"]";
		out << ";\n";
	}
	//Add all of its edges:
	for (local_stemma_edge e : graph.edges) {
		string prior = e.prior;
		string posterior = e.posterior;
		out << "\t";
		out << id_to_index.at(prior) << " -> " << id_to_index.at(posterior);
		out << ";\n";
	}
	out << "}" << endl;
	return;
}
