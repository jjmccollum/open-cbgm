/*
 * textual_flow.cpp
 *
 *  Created on: Dec 24, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <cstring>
#include <string>
#include <list>
#include <vector>
#include <set> //for small sets keyed by readings
#include <unordered_set> //for large sets keyed by witnesses
#include <map> //for small maps keyed by readings
#include <unordered_map> //for large maps keyed by witnesses
#include <limits>

#include "pugixml.h"
#include "roaring.hh"
#include "textual_flow.h"
#include "witness.h"
#include "variation_unit.h"

using namespace std;

/**
 * Default constructor.
 */
textual_flow::textual_flow() {

}

/**
 * Constructs a textual flow instance from a variation unit
 * and a list of witnesses whose potential ancestors have been set.
 */
textual_flow::textual_flow(const variation_unit & vu, const list<witness> & witnesses) {
	//Copy the label, readings, and connectivity from the variation unit:
	label = vu.get_label();
	readings = vu.get_readings();
	connectivity = vu.get_connectivity();
	//Get the variation unit's local stemma:
	local_stemma ls = vu.get_local_stemma();
	//Initialize the textual flow graph as empty:
	graph.vertices = list<textual_flow_vertex>();
	graph.edges = list<textual_flow_edge>();
	//Get a copy of the variation unit's reading support map:
	unordered_map<string, string> reading_support = vu.get_reading_support();
	//Add vertices and edges for each witness in the input list:
	for (witness wit : witnesses) {
		//Get the witness's ID and a list of any readings it has at this variation unit:
		string wit_id = wit.get_id();
		string wit_rdg = reading_support.find(wit_id) != reading_support.end() ? reading_support.at(wit_id) : "";
		//Add a vertex for this witness to the graph:
		textual_flow_vertex v;
		v.id = wit_id;
		v.rdg = wit_rdg;
		graph.vertices.push_back(v);
		//If this witness has no potential ancestors (i.e., if it has equal priority to the Ausgangstext),
		//then there are no edges to add, and we can continue:
		list<string> potential_ancestor_ids = wit.get_potential_ancestor_ids();
		if (potential_ancestor_ids.empty()) {
			continue;
		}
		//Otherwise, proceed to identify this witness's textual flow ancestor for this variation unit:
		string textual_flow_ancestor_id;
		int con = -1;
		int con_value = -1; //connectivity rank only changes when this value changes
		flow_type type = flow_type::NONE;
		//If the witness is extant, then attempt to find an ancestor with an equal or prior reading:
		if (!wit_rdg.empty()) {
			//If there is a potential ancestor within the connectivity limit that agrees with this witness,
			//then it is the textual flow ancestor:
			con = -1;
			con_value = -1;
			for (string potential_ancestor_id : potential_ancestor_ids) {
				//Update the connectivity rank if the connectivity value changes:
				genealogical_comparison comp = wit.get_genealogical_comparison_for_witness(potential_ancestor_id);
				int agreements = comp.agreements.cardinality();
				if (agreements != con_value) {
					con_value = agreements;
					con++;
				}
				//If we reach the connectivity limit, then exit the loop early:
				if (con == connectivity) {
					break;
				}
				//If this potential ancestor agrees with the current witness here, then we're done:
				bool agree = false;
				if (reading_support.find(potential_ancestor_id) != reading_support.end()) {
					string potential_ancestor_rdg = reading_support.at(potential_ancestor_id);
					if (ls.path_exists(potential_ancestor_rdg, wit_rdg) && ls.get_shortest_path_length(potential_ancestor_rdg, wit_rdg) == 0) {
						agree = true;
					}
				}
				if (agree) {
					textual_flow_ancestor_id = potential_ancestor_id;
					type = flow_type::EQUAL;
					break;
				}
			}
		}
		//If the witness is lacunose or it does not have a potential ancestor with its reading within the connectivity limit,
		//then its first potential ancestor is its textual flow ancestor:
		if (textual_flow_ancestor_id.empty()) {
			con = 0;
			textual_flow_ancestor_id = potential_ancestor_ids.front();
			string textual_flow_ancestor_rdg = reading_support.find(textual_flow_ancestor_id) != reading_support.end() ? reading_support.at(textual_flow_ancestor_id) : "";
			type = wit_rdg.empty() || textual_flow_ancestor_rdg.empty() ? flow_type::LOSS : flow_type::CHANGE;
		}
		//Calculate the strength of the textual flow from the textual flow ancestor to its descendant
		//based on relative proportion of prior readings to extant readings:
		genealogical_comparison self_comp = wit.get_genealogical_comparison_for_witness(wit_id);
		genealogical_comparison ancestor_comp = wit.get_genealogical_comparison_for_witness(textual_flow_ancestor_id);
		Roaring primary_extant = self_comp.explained;
		Roaring agreements = ancestor_comp.agreements;
		Roaring explained = ancestor_comp.explained;
		float strength = float((explained ^ agreements).cardinality()) / float(primary_extant.cardinality());
		//Add an edge to the graph connecting the current witness to its textual flow ancestor:
		textual_flow_edge e;
		e.descendant = wit_id;
		e.ancestor = textual_flow_ancestor_id;
		e.type = type;
		e.connectivity = con;
		e.strength = strength;
		graph.edges.push_back(e);
	}
}

/**
 * Default destructor.
 */
textual_flow::~textual_flow() {

}

/**
 * Returns the label of this textual_flow instance.
 */
string textual_flow::get_label() const {
	return label;
}

/**
 * Returns the readings list of this textual_flow_instance.
 */
list<string> textual_flow::get_readings() const {
	return readings;
}

/**
 * Returns the connectivity of this textual_flow instance.
 */
int textual_flow::get_connectivity() const {
	return connectivity;
}

/**
 * Returns the textual flow diagram of this textual_flow instance.
 */
textual_flow_graph textual_flow::get_graph() const {
	return graph;
}

/**
 * Given an output stream, writes a complete textual flow diagram to output in .dot format.
 * An optional flag indicating whether to format edges to reflect flow strength can also be specified.
 */
void textual_flow::textual_flow_to_dot(ostream & out, bool flow_strengths=false) {
	//Add the graph first:
	out << "digraph textual_flow {\n";
	//Add a line indicating that nodes do not have any shape:
	out << "\tnode [shape=plaintext];\n";
	//Add a box node indicating the label of this variation_unit:
	out << "\tlabel [shape=box, label=\"" << label << "\\nCon = " << (connectivity == numeric_limits<int>::max() ? "Absolute" : to_string(connectivity)) << "\"];\n";
	//Add all of the graph nodes, keeping track of their numerical indices:
	unordered_map<string, int> id_to_index = unordered_map<string, int>();
	for (textual_flow_vertex v : graph.vertices) {
		//Map the ID of this vertex to its numerical index:
		string wit_id = v.id;
		string wit_rdg = v.rdg;
		id_to_index[wit_id] = id_to_index.size();
		int wit_ind = id_to_index.at(wit_id);
		//Then add the node:
		out << "\t" << wit_ind;
		//Format the node based on its readings list:
		if (wit_rdg.empty()) {
			//The witness is lacunose at this variation unit:
			out << " [label=\"" << wit_id << "\", color=gray, shape=ellipse, style=dashed]";
		}
		else {
			//The witness has a reading at this variation unit:
			out << " [label=\"" << wit_id << " (" << wit_rdg << ")\", shape=ellipse]";
		}
		out << ";\n";
	}
	//Add all of the graph edges:
	for (textual_flow_edge e : graph.edges) {
		//Get the indices corresponding to the endpoints' IDs:
		int ancestor_ind = id_to_index.at(e.ancestor);
		int descendant_ind = id_to_index.at(e.descendant);
		//Handle the conditional formatting of the edge:
		list<string> format_cmds = list<string>();
		//If the connectivity index is not direct (i.e., 0), then print it in one-based format:
		if (e.connectivity > 0) {
			string edge_label = "label=\"" + to_string(e.connectivity + 1) + "\", fontsize=10";
			format_cmds.push_back(edge_label);
		}
		//Format the color based on the flow type:
		if (e.type == flow_type::EQUAL) {
			string edge_color = "color=black";
			format_cmds.push_back(edge_color);
		}
		else if (e.type == flow_type::CHANGE) {
			string edge_color = "color=blue";
			format_cmds.push_back(edge_color);
		}
		else if (e.type == flow_type::LOSS) {
			string edge_color = "color=gray";
			format_cmds.push_back(edge_color);
		}
		if (flow_strengths) {
			//Format the line style based on the flow strength:
			if (e.strength <= 0.01) {
				string edge_style = "style=dotted";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength <= 0.05) {
				string edge_style = "style=dashed";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength <= 0.1) {
				string edge_style = "penwidth=1.0";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength <= 0.25) {
				string edge_style = "penwidth=2.0";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength <= 0.5) {
				string edge_style = "penwidth=3.0";
				format_cmds.push_back(edge_style);
			}
			else {
				string edge_style = "penwidth=4.0";
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

/**
 * Given a reading ID and an output stream,
 * writes a coherence in attestations diagram for that reading to output in .dot format.
 * An optional flag indicating whether to format edges to reflect flow strength can also be specified.
 */
void textual_flow::coherence_in_attestations_to_dot(ostream & out, const string & rdg, bool flow_strengths=false) {
	//Add the graph first:
	out << "digraph textual_flow_diagram {\n";
	//Add a line indicating that nodes do not have any shape:
	out << "\tnode [shape=plaintext];\n";
	//Add a box node indicating the label of this variation_unit and the selected reading:
	out << "\tlabel [shape=box, label=\"" << label << rdg << "\\nCon = " << (connectivity == numeric_limits<int>::max() ? "Absolute" : to_string(connectivity)) << "\"];\n";
	//Maintain a map of node IDs to numerical indices
	//and a vector of vertex data structures:
	unordered_map<string, int> id_to_index = unordered_map<string, int>();
	vector<textual_flow_vertex> vertices = vector<textual_flow_vertex>();
	for (textual_flow_vertex v : graph.vertices) {
		//Map the ID of this vertex to its numerical index:
		string wit_id = v.id;
		id_to_index[wit_id] = id_to_index.size();
		vertices.push_back(v);
	}
	//Now draw the primary set of vertices corresponding to witnesses with the input reading:
	unordered_set<string> primary_set = unordered_set<string>();
	for (textual_flow_vertex v : graph.vertices) {
		//If this witness does not have the specified reading, then skip it:
		string wit_id = v.id;
		string wit_rdg = v.rdg;
		if (wit_rdg != rdg) {
			continue;
		}
		//Otherwise, draw a vertex with its numerical index:
		int wit_ind = id_to_index.at(wit_id);
		//Then add the node:
		out << "\t" << wit_ind;
		out << " [label=\"" << wit_id << " (" << wit_rdg << ")\", shape=ellipse]";
		out << ";\n";
		primary_set.insert(wit_id);
	}
	//Then add a secondary set of vertices for ancestors of these witnesses with a different reading:
	unordered_set<string> secondary_set = unordered_set<string>();
	for (textual_flow_edge e : graph.edges) {
		//Get the endpoints' IDs:
		string ancestor_id = e.ancestor;
		string descendant_id = e.descendant;
		//If the descendant is not in the primary vertex set, or the ancestor is, then skip:
		if (primary_set.find(descendant_id) == primary_set.end() || primary_set.find(ancestor_id) != primary_set.end()) {
			continue;
		}
		//Otherwise, we have an ancestor with a reading other than the specified one;
		//skip it if we've added it already:
		if (secondary_set.find(ancestor_id) != secondary_set.end()) {
			continue;
		}
		//If it's new, then add a vertex for it:
		int ancestor_ind = id_to_index.at(ancestor_id);
		textual_flow_vertex v = vertices[ancestor_ind];
		string ancestor_rdg = v.rdg;
		out << "\t" << ancestor_ind;
		out << " [label=\"" << ancestor_id << " (" << ancestor_rdg << ")\", color=blue, shape=ellipse, type=dashed]";
		out << ";\n";
		secondary_set.insert(ancestor_id);
	}
	//Add all of the graph edges:
	for (textual_flow_edge e : graph.edges) {
		//Get the endpoints' IDs:
		string ancestor_id = e.ancestor;
		string descendant_id = e.descendant;
		//If the descendant is not in the primary vertex set, then skip this edge:
		if (primary_set.find(descendant_id) == primary_set.end()) {
			continue;
		}
		//Otherwise, get the indices of the endpoints:
		int ancestor_ind = id_to_index.at(ancestor_id);
		int descendant_ind = id_to_index.at(descendant_id);
		//Handle the conditional formatting of the edge:
		list<string> format_cmds = list<string>();
		//If the connectivity index is not direct (i.e., 0), then print it in one-based format:
		if (e.connectivity > 0) {
			string edge_label = "label=\"" + to_string(e.connectivity + 1) + "\", fontsize=10";
			format_cmds.push_back(edge_label);
		}
		//Format the color based on the flow type:
		if (e.type == flow_type::EQUAL) {
			string edge_color = "color=black";
			format_cmds.push_back(edge_color);
		}
		else if (e.type == flow_type::CHANGE) {
			string edge_color = "color=blue";
			format_cmds.push_back(edge_color);
		}
		else if (e.type == flow_type::LOSS) {
			string edge_color = "color=gray";
			format_cmds.push_back(edge_color);
		}
		if (flow_strengths) {
			//Format the line style based on the flow strength:
			if (e.strength <= 0.01) {
				string edge_style = "style=dotted";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength <= 0.05) {
				string edge_style = "style=dashed";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength <= 0.1) {
				string edge_style = "penwidth=1.0";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength <= 0.25) {
				string edge_style = "penwidth=2.0";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength <= 0.5) {
				string edge_style = "penwidth=3.0";
				format_cmds.push_back(edge_style);
			}
			else {
				string edge_style = "penwidth=4.0";
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

/**
 * Given an output stream, writes a coherence in variant passages diagram to output in .dot format.
 * An optional flag indicating whether to format edges to reflect flow strength can also be specified.
 */
void textual_flow::coherence_in_variant_passages_to_dot(ostream & out, bool flow_strengths=false) {
	//Add the graph first:
	out << "digraph textual_flow_diagram {\n";
	//Add a line indicating that nodes do not have any shape:
	out << "\tnode [shape=plaintext];\n";
	//Add a box node indicating the label of this variation_unit:
	out << "\tlabel [shape=box, label=\"" << label << "\\nCon = " << (connectivity == numeric_limits<int>::max() ? "Absolute" : to_string(connectivity)) << "\"];\n";
	//Maintain a map of node IDs to numerical indices
	//and a vector of vertex data structures:
	unordered_map<string, int> id_to_index = unordered_map<string, int>();
	vector<textual_flow_vertex> vertices = vector<textual_flow_vertex>();
	for (textual_flow_vertex v : graph.vertices) {
		//Map the ID of this vertex to its numerical index:
		string wit_id = v.id;
		id_to_index[wit_id] = id_to_index.size();
		vertices.push_back(v);
	}
	//Maintain a map of support lists for each reading:
	map<string, list<string>> clusters = map<string, list<string>>();
	for (string rdg : readings) {
		clusters[rdg] = list<string>();
	}
	for (textual_flow_vertex v : graph.vertices) {
		string wit_id = v.id;
		string wit_rdg = v.rdg;
		clusters[wit_rdg].push_back(wit_id);
	}
	//Maintain a set of IDs for nodes between which there exists an edge of flow type CHANGE:
	unordered_set<string> change_wit_ids = unordered_set<string>();
	for (textual_flow_edge e : graph.edges) {
		if (e.type == flow_type::CHANGE) {
			change_wit_ids.insert(e.ancestor);
			change_wit_ids.insert(e.descendant);
		}
	}
	//Add a cluster for each reading, including all of the nodes it contains:
	for (string rdg : readings) {
		list<string> cluster = clusters.at(rdg);
		out << "\tsubgraph cluster_" << rdg << " {\n";
		out << "\t\tlabeljust=\"c\";\n";
		out << "\t\tlabel=\"" << rdg << "\";\n";
		for (string wit_id : cluster) {
			//If this witness is not at either end of a CHANGE flow edge, then skip it:
			if (change_wit_ids.find(wit_id) == change_wit_ids.end()) {
				continue;
			}
			//Otherwise, add a vertex for it:
			int wit_ind = id_to_index.at(wit_id);
			out << "\t\t" << wit_ind;
			out << " [label=\"" << wit_id << "\", shape=ellipse]";
			out << ";\n";
		}
		out << "\t}\n";
	}
	//Finally, add the "CHANGE" edges:
	for (textual_flow_edge e : graph.edges) {
		//Only add "CHANGE" edges:
		if (e.type != flow_type::CHANGE) {
			continue;
		}
		//Get the indices corresponding to the endpoints' IDs:
		int ancestor_ind = id_to_index.at(e.ancestor);
		int descendant_ind = id_to_index.at(e.descendant);
		//Handle the conditional formatting of the edge:
		list<string> format_cmds = list<string>();
		//If the connectivity index is not direct (i.e., 0), then print it in one-based format:
		if (e.connectivity > 0) {
			string edge_label = "label=\"" + to_string(e.connectivity + 1) + "\", fontsize=10";
			format_cmds.push_back(edge_label);
		}
		//Format the color based on the flow type:
		if (e.type == flow_type::EQUAL) {
			string edge_color = "color=black";
			format_cmds.push_back(edge_color);
		}
		else if (e.type == flow_type::CHANGE) {
			string edge_color = "color=blue";
			format_cmds.push_back(edge_color);
		}
		else if (e.type == flow_type::LOSS) {
			string edge_color = "color=gray";
			format_cmds.push_back(edge_color);
		}
		if (flow_strengths) {
			//Format the line style based on the flow strength:
			if (e.strength <= 0.01) {
				string edge_style = "style=dotted";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength <= 0.05) {
				string edge_style = "style=dashed";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength <= 0.1) {
				string edge_style = "penwidth=1.0";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength <= 0.25) {
				string edge_style = "penwidth=2.0";
				format_cmds.push_back(edge_style);
			}
			else if (e.strength <= 0.5) {
				string edge_style = "penwidth=3.0";
				format_cmds.push_back(edge_style);
			}
			else {
				string edge_style = "penwidth=4.0";
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
