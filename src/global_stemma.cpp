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
#include "variation_unit.h"
#include "apparatus.h"
#include "witness.h"

using namespace std;

/**
 * Default constructor.
 */
global_stemma::global_stemma() {

}

/**
 * Constructs a global stemma from an apparatus and a map of witnesses keyed by ID.
 */
global_stemma::global_stemma(apparatus & app, unordered_map<string, witness> & witnesses_by_id) {
	graph.vertices = list<global_stemma_vertex>();
	graph.edges = list<global_stemma_edge>();
	//Ensure that each variation unit's textual flow has been calculated:
	for (variation_unit vu : app.get_variation_units()) {
		vu.calculate_textual_flow(witnesses_by_id);
	}
	//Create a vertex for each witness:
	for (pair<string, witness> kv : witnesses_by_id) {
		string wit_id = kv.first;
		global_stemma_vertex v;
		v.id = wit_id;
		graph.vertices.push_back(v);
	}
	//Now that each witness has its set of textual flow ancestors populated,
	//retrieve the ancestors in their optimal substemmata and add the appropriate edges:
	for (pair<string, witness> kv : witnesses_by_id) {
		string wit_id = kv.first;
		witness wit = kv.second;
		//The ausgangstext will not have any ancestors, so we can skip it:
		if (wit.get_potential_ancestor_ids().size() == 0) {
			continue;
		}
		unordered_set<string> global_stemma_ancestors = wit.get_global_stemma_ancestors();
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
			e.ambiguous = (wit.get_explained_readings_for_witness(ancestor_id).cardinality() == ancestor.get_explained_readings_for_witness(wit_id).cardinality());
			graph.edges.push_back(e);
		}
	}
}

global_stemma::~global_stemma() {

}

/**
 * Given an output stream, writes the global stemma graph to output in .dot format.
 */
void global_stemma::to_dot(ostream & out) {
	//Add the graph first:
	out << "digraph global_stemma {\n";
	//Add lines specifying the font and font size:
	out << "\tgraph [fontname = \"helvetica\", fontsize=15];\n";
	out << "\tnode [fontname = \"helvetica\", fontsize=15];\n";
	out << "\tedge [fontname = \"helvetica\", fontsize=15];\n";
	//Add a line indicating that nodes do not have any shape:
	out << "\tnode [shape=plaintext];\n";
	//Add a box node indicating the label of this variation_unit:
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
		bool ambiguous = e.ambiguous;
		out << "\t";
		//Use a dashed, undirected edge if the relationship is ambiguous:
		if (ambiguous) {
			out << ancestor_index << " -- " << descendant_index;
			out << " [style=dashed, penwidth=" << weight << "]";
		}
		//Otherwise, use a solid, directed edge:
		else {
			out << ancestor_index << " -> " << descendant_index;
			out << " [penwidth=" << weight << ", arrowsize=" << weight << "]";
		}
		out << ";\n";
	}
	out << "}" << endl;
	return;
}
