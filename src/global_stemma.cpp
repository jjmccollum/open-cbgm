/*
 * global_stemma.cpp
 *
 *  Created on: Nov 14, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "global_stemma.h"
#include "witness.h"

using namespace std;

/**
 * Default constructor.
 */
global_stemma::global_stemma() {

}

/**
 * Constructs a global stemma from a map of witnesses keyed by ID.
 * The witnesses are assumed to have their lists of global stemma ancestor populated.
 */
global_stemma::global_stemma(unordered_map<string, witness> witnesses_by_id) {
	graph.vertices = list<global_stemma_vertex>();
	graph.edges = list<global_stemma_edge>();
	//Create a vertex for each witness:
	for (pair<string, witness> kv : witnesses_by_id) {
		string wit_id = kv.first;
		global_stemma_vertex v;
		v.id = wit_id;
		graph.vertices.push_back(v);
	}
	//Now that each witness has its global stemma ancestors determined,
	//retrieve the ancestors in their optimal substemmata and add the appropriate edges:
	for (pair<string, witness> kv : witnesses_by_id) {
		string wit_id = kv.first;
		witness wit = kv.second;
		//Skip any witnesses with no global stemma ancestors (such as the Ausgangstext and highly lacunose witnesses):
		list<string> global_stemma_ancestors = wit.get_global_stemma_ancestors();
		if (global_stemma_ancestors.empty()) {
			continue;
		}
		//Get the maximum number of agreements between this witness and its ancestors:
		int max_agreements = 0;
		for (string ancestor_id : global_stemma_ancestors) {
			int agreements = wit.get_agreements_for_witness(ancestor_id).cardinality();
			if (agreements > max_agreements) {
				max_agreements = agreements;
			}
		}
		//Now, add an edge for each ancestor:
		for (string ancestor_id : global_stemma_ancestors) {
			witness ancestor = witnesses_by_id[ancestor_id];
			global_stemma_edge e;
			e.ancestor = ancestor_id;
			e.descendant = wit_id;
			e.weight = float(wit.get_agreements_for_witness(ancestor_id).cardinality()) / float(max_agreements);
			graph.edges.push_back(e);
		}
	}
}

/**
 * Default destructor.
 */
global_stemma::~global_stemma() {

}

/**
 * Returns the graph representing this global stemma.
 */
global_stemma_graph global_stemma::get_graph() {
	return graph;
}

/**
 * Given an output stream, writes the global stemma graph to output in .dot format.
 */
void global_stemma::to_dot(ostream & out) {
	//Add the graph first:
	out << "digraph global_stemma {\n";
	//Add a line indicating that nodes do not have any shape:
	out << "\tnode [shape=plaintext];\n";
	//Add a box node indicating the label of this graph:
	out << "\tlabel [shape=box, label=\"Global Stemma\"];\n";
	//Add all of its nodes, mapping their IDs to numerical indices:
	unordered_map<string, int> id_to_index = unordered_map<string, int>();
	for (global_stemma_vertex v : graph.vertices) {
		string wit_id = v.id;
		id_to_index[wit_id] = id_to_index.size();
		int index = id_to_index[wit_id];
		out << "\t";
		out << index << " [label=\"" << wit_id << "\"]";
		out << ";\n";
	}
	//Add all of its edges:
	for (global_stemma_edge e : graph.edges) {
		//Get the numerical indices of the endpoints:
		string ancestor_id = e.ancestor;
		string descendant_id = e.descendant;
		int ancestor_index = id_to_index[ancestor_id];
		int descendant_index = id_to_index[descendant_id];
		float weight = e.weight;
		out << "\t";
		out << ancestor_index << " -> " << descendant_index;
		out << " [penwidth=" << weight << ", arrowsize=" << weight << "]";
		out << ";\n";
	}
	out << "}" << endl;
	return;
}
