/*
 * local_stemma.cpp
 *
 *  Created on: Oct 31, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <list>

#include "pugixml.h"
#include "roaring.hh"
#include "local_stemma.h"

using namespace std;

/**
 * Recursive function for populating a transitive closure adjacency matrix given an adjacency list for a graph.
 */
void transitive_closure_dfs(int i, int j, vector<list<int>> adjacency_list, vector<Roaring> & closure_matrix) {
	closure_matrix[i].add(j);
	for (int k : adjacency_list[j]) {
		if (!closure_matrix[i].contains(k)) {
			transitive_closure_dfs(i, k, adjacency_list, closure_matrix);
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
 */
local_stemma::local_stemma(string apparatus_label, const pugi::xml_node xml) {
	graph.vertices = list<local_stemma_vertex>();
	graph.edges = list<local_stemma_edge>();
	//Set the label:
	label = apparatus_label;
	//Iterate through the <graph/> element's children,
	//storing their IDs and maintaining a map of these IDs to numerical indices:
	unordered_map<string, int> id_to_ind = unordered_map<string, int>();
	for (pugi::xml_node node : xml.children("node")) {
		//Map node "n" attributes to their indices:
		string id = node.attribute("n").value();
		id_to_ind[id] = id_to_ind.size();
		//Then create a vertex and add it to the graph:
		local_stemma_vertex v;
		v.id = id;
		v.index = id_to_ind[id];
		graph.vertices.push_back(v);
	}
	for (pugi::xml_node arc : xml.children("arc")) {
		//Resolve the IDs of the edges' sources and targets, and add them to the edges list:
		string from_id = arc.attribute("from").value();
		string to_id = arc.attribute("to").value();
		int from = id_to_ind[from_id];
		int to = id_to_ind[to_id];
		//Then create an edge and add it to the graph:
		local_stemma_edge e;
		e.from = from;
		e.to = to;
		graph.edges.push_back(e);
	}
	//Construct an adjacency list from the graph:
	vector<list<int>> adjacency_list = vector<list<int>>(graph.vertices.size());
	for (local_stemma_edge e : graph.edges) {
		adjacency_list[e.from].push_back(e.to);
	}
	//Now use depth-first traversal to populate the adjacency matrix of the transitive closure:
	closure_matrix = vector<Roaring>(graph.vertices.size());
	for (local_stemma_vertex v : graph.vertices) {
		int index = v.index;
		//Recursively mark all vertices reachable from this one, including this vertex itself:
		transitive_closure_dfs(index, index, adjacency_list, closure_matrix);
	}
}

local_stemma::~local_stemma() {

}

/**
 * Returns the label for this local_stemma.
 */
string local_stemma::get_label() {
	return label;
}

/**
 * Returns the graph structure for this local_stemma.
 */
local_stemma_graph local_stemma::get_graph() {
	return graph;
}

/**
 * Return the adjacency matrix of the transitive closure of this local_stemma's graph.
 */
vector<Roaring> local_stemma::get_closure_matrix() {
	return closure_matrix;
}

/**
 * Given two reading indices, checks if they are equal
 * or if the first reading is prior to the second
 * (i.e., if the first reading is adjacent to the second in the transitive closure of the stemma graph).
 */
bool local_stemma::is_equal_or_prior(int i, int j) {
	return closure_matrix[i].contains(j);
}

/**
 * Given an output stream, writes the local stemma graph to output in .dot format.
 */
void local_stemma::to_dot(ostream & out) {
	//Add the graph first:
	out << "digraph local_stemma {\n";
	//Add lines specifying the font and font size:
	out << "\tgraph [fontname = \"helvetica\", fontsize=15];\n";
    out << "\tnode [fontname = \"helvetica\", fontsize=15];\n";
    out << "\tedge [fontname = \"helvetica\", fontsize=15];\n";
	//Add a line indicating that nodes do not have any shape:
	out << "\tnode [shape=plaintext];\n";
	//Add a box node indicating the label of this variation_unit:
	out << "\tlabel [shape=box, label=\"" << label << "\"];\n";
	//Add all of its nodes:
	for (local_stemma_vertex v : graph.vertices) {
		out << "\t";
		out << v.index;
		if (!v.id.empty()) {
			//Use the ID as the node label, if it exists:
			out << " [label=\"" << v.id << "\"]";
		}
		out << ";\n";
	}
	//Add all of its edges:
	for (local_stemma_edge e : graph.edges) {
		out << "\t";
		out << e.from << " -> " << e.to;
		out << ";\n";
	}
	out << "}" << endl;
	return;
}
