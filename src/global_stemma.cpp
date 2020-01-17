/*
 * global_stemma.cpp
 *
 *  Created on: Nov 14, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>
#include <list>
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
 * Constructs a global stemma from a list of witnesses.
 * The witnesses are assumed to have their lists of global stemma ancestor populated.
 */
global_stemma::global_stemma(const list<witness> & witnesses) {
	graph.vertices = list<global_stemma_vertex>();
	graph.edges = list<global_stemma_edge>();
	//Create a map of the witnesses, keyed by ID:
	unordered_map<string, witness> witnesses_by_id = unordered_map<string, witness>();
	for (witness wit : witnesses) {
		string wit_id = wit.get_id();
		witnesses_by_id[wit_id] = wit;
	}
	//Create a vertex for each witness:
	for (witness wit : witnesses) {
		string wit_id = wit.get_id();
		global_stemma_vertex v;
		v.id = wit_id;
		graph.vertices.push_back(v);
	}
	//Now that each witness has its global stemma ancestors determined,
	//retrieve the ancestors in their optimal substemmata and add the appropriate edges:
	for (witness wit : witnesses) {
		string wit_id = wit.get_id();
		//Skip any witnesses with no global stemma ancestors (such as the Ausgangstext and highly lacunose witnesses):
		list<string> global_stemma_ancestor_ids = wit.get_global_stemma_ancestor_ids();
		if (global_stemma_ancestor_ids.empty()) {
			continue;
		}
		//Get the number of extant passages for this witness:
		unsigned int extant = wit.get_genealogical_comparison_for_witness(wit_id).explained.cardinality();
		//Now, add an edge for each ancestor:
		for (string ancestor_id : global_stemma_ancestor_ids) {
			witness ancestor = witnesses_by_id.at(ancestor_id);
			genealogical_comparison comp = wit.get_genealogical_comparison_for_witness(ancestor_id);
			global_stemma_edge e;
			e.ancestor = ancestor_id;
			e.descendant = wit_id;
			e.weight = float(comp.agreements.cardinality()) / float(extant);
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
global_stemma_graph global_stemma::get_graph() const {
	return graph;
}

/**
 * Given an output stream, writes the global stemma graph to output in .dot format.
 * An optional flag indicating whether to format edges to reflect proportions of agreements with stemmatic ancestors can also be specified.
 */
void global_stemma::to_dot(ostream & out, bool format_edges=false) {
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
		int index = id_to_index.at(wit_id);
		out << "\t";
		out << index << " [label=\"" << wit_id << "\", shape=ellipse]";
		out << ";\n";
	}
	//Add all of its edges:
	for (global_stemma_edge e : graph.edges) {
		//Get the numerical indices of the endpoints:
		string ancestor_id = e.ancestor;
		string descendant_id = e.descendant;
		int ancestor_ind = id_to_index.at(ancestor_id);
		int descendant_ind = id_to_index.at(descendant_id);
		//Handle the conditional formatting of the edge:
		list<string> format_cmds = list<string>();
		if (format_edges) {
			//Format the line style based on the flow strength:
			if (e.weight <= 0.5) {
				string edge_style = "style=dotted";
				format_cmds.push_back(edge_style);
			}
			else if (e.weight <= 0.75) {
				string edge_style = "style=dashed";
				format_cmds.push_back(edge_style);
			}
			else {
				string edge_style = "style=solid";
				format_cmds.push_back(edge_style);
			}
		}
		//Add a line describing the edge:
		out << "\t" << ancestor_ind << " -> " << descendant_ind << " [";
		for (string format_cmd : format_cmds) {
			if (format_cmd != format_cmds.front()) {
				out << ", ";
			}
			out << format_cmd;
		}
		out << "];\n";
	}
	out << "}" << endl;
	return;
}
