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
 * Populates a given shortest paths map
 * by performing breadth-first search on the given graph (in map-of-neighbors format).
 */
void populate_shortest_paths(const map<string, list<string>> & adjacency_map, map<pair<string, string>, int> & shortest_paths) {
	//Proceed for each vertex:
	for (pair<string, list<string>> kv : adjacency_map) {
		string s = kv.first;
		//Initialize a queue of vertex IDs to process, starting with the current source vertex:
		list<string> queue = list<string>({s});
		//Add a shortest path of length 0 from the current source to itself:
		shortest_paths[pair<string, string>(s, s)] = 0;
		//Then proceed by breadth-first search:
		while (!queue.empty()) {
			//Pop off the vertex at the front of the queue:
			string u = queue.front();
			queue.pop_front();
			//Get the length of the shortest path from the source to this vertex:
			int dist = shortest_paths.at(pair<string, string>(s, u));
			//Then proceed for each vertex adjacent to it:
			for (string v : adjacency_map.at(u)) {
				//Check if the path from the source to this vertex has already been processed:
				pair<string, string> p = pair<string, string>(s, v);
				if (shortest_paths.find(p) == shortest_paths.end()) {
					//If it hasn't, then add its length to the shortest paths map:
					shortest_paths[p] = dist + 1;
					//Then add the vertex itself to the queue:
					queue.push_back(v);
				}
			}
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
	//Now use breadth-first traversal to populate the map of shortest paths:
	shortest_paths = map<pair<string, string>, int>();
	populate_shortest_paths(adjacency_map, shortest_paths);
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
 * Return the map of shortest paths for this local_stemma's graph.
 */
map<pair<string, string>, int> local_stemma::get_shortest_paths() const {
	return shortest_paths;
}

/**
 * Given two reading IDs, checks if a path exists between them in the local stemma.
 */
bool local_stemma::path_exists(const string & r1, const string & r2) const {
	return shortest_paths.find(pair<string, string>(r1, r2)) != shortest_paths.end();
}

/**
 * Given two reading IDs, returns the length of the shortest path between them in the local stemma.
 * It is assumed that a path exists between the two readings.
 */
int local_stemma::get_shortest_path_length(const string & r1, const string & r2) const {
	return shortest_paths.at(pair<string, string>(r1, r2));
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
