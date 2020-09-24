/*
 * global_stemma.cpp
 *
 *  Created on: Nov 14, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <list>
#include <unordered_map>
#include <unordered_set>

#include "roaring.hh"
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
 * The witnesses are assumed to have their lists of stemmatic ancestors populated.
 */
global_stemma::global_stemma(const list<witness> & witnesses) {
	vertices = list<global_stemma_vertex>();
	edges = list<global_stemma_edge>();
	//Create a vertex for each witness:
	for (witness wit : witnesses) {
		string wit_id = wit.get_id();
		global_stemma_vertex v;
		v.id = wit_id;
		vertices.push_back(v);
	}
	//Retrieve each witness's stemmatic ancestors and add the appropriate edges:
	for (witness wit : witnesses) {
		string wit_id = wit.get_id();
		//If the witness has no stemmatic ancestors (which happens for the Ausgangstext and highly lacunose witnesses),
		//then no edges need to be drawn to it:
		list<string> stemmatic_ancestor_ids = wit.get_stemmatic_ancestor_ids();
		if (stemmatic_ancestor_ids.empty()) {
			continue;
		}
		//Otherwise, add an edge for each ancestor:
		for (string ancestor_id : stemmatic_ancestor_ids) {
			//Calculate the genealogical cost and stability of the textual flow:
			genealogical_comparison comp = wit.get_genealogical_comparison_for_witness(ancestor_id);
			float length = comp.cost;
			float strength = float(comp.posterior.cardinality() - comp.prior.cardinality()) / float(comp.extant.cardinality());
			global_stemma_edge e;
			e.ancestor = ancestor_id;
			e.descendant = wit_id;
			e.length = length;
			e.strength = strength;
			edges.push_back(e);
		}
	}
}

/**
 * Default destructor.
 */
global_stemma::~global_stemma() {

}

/**
 * Returns this global stemma's list of vertices.
 */
list<global_stemma_vertex> global_stemma::get_vertices() const {
	return vertices;
}

/**
 * Returns this global stemma's list of edges.
 */
list<global_stemma_edge> global_stemma::get_edges() const {
	return edges;
}

/**
 * Given an output stream, writes the global stemma graph to output in .dot format.
 * Optional flags indicating whether to print edge lengths and format edges based on flow strength can be specified.
 */
void global_stemma::to_dot(ostream & out, bool print_lengths, bool flow_strengths) {
	//Add the graph first:
	out << "digraph global_stemma {\n";
	//Add a subgraph for the legend:
	out << "\tsubgraph cluster_legend {\n";
	//Add a box node indicating the label of this graph:
	out << "\t\tlabel [shape=plaintext, label=\"Global Stemma\"];\n";
	out << "\t}\n";
	//Add a subgraph for the plot:
	out << "\tsubgraph cluster_plot {\n";
	//Make its border invisible:
	out << "\t\tstyle=invis;\n";
	//Add a line indicating that nodes have an ellipse shape:
	out << "\t\tnode [shape=ellipse];\n";
	//Add all of its nodes, mapping their IDs to numerical indices:
	unordered_map<string, int> id_to_index = unordered_map<string, int>();
	for (global_stemma_vertex v : vertices) {
		string wit_id = v.id;
		unsigned int i = (unsigned int) id_to_index.size();
		id_to_index[wit_id] = i;
		int index = id_to_index.at(wit_id);
		out << "\t\t";
		out << index << " [label=\"" << wit_id << "\"]";
		out << ";\n";
	}
	//Add all of its edges:
	for (global_stemma_edge e : edges) {
		//Get the numerical indices of the endpoints:
		string ancestor_id = e.ancestor;
		string descendant_id = e.descendant;
		int ancestor_ind = id_to_index.at(ancestor_id);
		int descendant_ind = id_to_index.at(descendant_id);
		//Handle the conditional formatting of the edge:
		list<string> format_cmds = list<string>();
		//Set the preferred length of the edge (used only by the neato and fdp Graphviz programs) based on its genealogical cost:
		string edge_len = "len=" + to_string(e.length);
		format_cmds.push_back(edge_len);
		//If the connectivity index is not direct (i.e., 0), then print it in one-based format:
		if (print_lengths) {
			stringstream ss;
			ss << "label=\"" << fixed << std::setprecision(3) << e.length << "\", fontsize=10" << endl;
			string edge_label = ss.str();
			format_cmds.push_back(edge_label);
		}
		if (flow_strengths) {
			//Format the line style based on the flow strength:
			if (e.strength < 0.01) {
				string edge_style = "style=dotted";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength < 0.05) {
				string edge_style = "style=dashed";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength < 0.1) {
				string edge_style = "style=solid";
				format_cmds.push_back(edge_style);
			}
			else {
				string edge_style = "style=bold";
				format_cmds.push_back(edge_style);
			}
		}
		//Add a line describing the edge:
		out << "\t\t" << ancestor_ind << " -> " << descendant_ind << " [";
		for (string format_cmd : format_cmds) {
			if (format_cmd != format_cmds.front()) {
				out << ", ";
			}
			out << format_cmd;
		}
		out << "];\n";
	}
	out << "\t}\n";
	out << "}" << endl;
	return;
}
