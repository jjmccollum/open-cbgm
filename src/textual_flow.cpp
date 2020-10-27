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
 * and a list of witnesses (whose lists of potential ancestors are assumed to be populated).
 */
textual_flow::textual_flow(const variation_unit & vu, const list<witness> & witnesses) {
	//Copy the label, readings, and connectivity from the variation unit:
	label = vu.get_label();
	readings = vu.get_readings();
	connectivity = vu.get_connectivity();
	//Get the variation unit's local stemma:
	local_stemma ls = vu.get_local_stemma();
	//Initialize the vertex and edge lists as empty:
	vertices = list<textual_flow_vertex>();
	edges = list<textual_flow_edge>();
	//Get a copy of the variation unit's reading support map:
	unordered_map<string, string> reading_support = vu.get_reading_support();
	//Create a map of witnesses, keyed by ID:
	unordered_map<string, witness> witnesses_by_id = unordered_map<string, witness>();
	for (witness wit : witnesses) {
		witnesses_by_id[wit.get_id()] = wit;
	}
	//Add vertices and edges for each witness in the input list:
	for (witness wit : witnesses) {
		//Get the witness's ID and its reading at this variation unit:
		string wit_id = wit.get_id();
		string wit_rdg = reading_support.find(wit_id) != reading_support.end() ? reading_support.at(wit_id) : "";
		//Add a vertex for this witness to the graph:
		textual_flow_vertex v;
		v.id = wit_id;
		v.rdg = wit_rdg;
		vertices.push_back(v);
		//If this witness has no potential ancestors (i.e., if it has equal priority to the Ausgangstext),
		//then there are no edges to add, and we can continue:
		list<string> potential_ancestor_ids = wit.get_potential_ancestor_ids();
		if (potential_ancestor_ids.empty()) {
			continue;
		}
		//Otherwise, proceed to identify this witness's textual flow ancestor for this variation unit:
		bool textual_flow_ancestor_found = false;
		int con = -1;
		int con_value = -1; //connectivity rank only changes when this value changes
		//If the witness is extant, then attempt to find an ancestor within the connectivity limit that agrees with it here:
		if (!wit_rdg.empty()) {
			con = -1;
			con_value = -1;
			for (string potential_ancestor_id : potential_ancestor_ids) {
				//Update the connectivity rank if the connectivity value changes:
				genealogical_comparison comp = wit.get_genealogical_comparison_for_witness(potential_ancestor_id);
				int agreements = (int) comp.agreements.cardinality();
				if (agreements != con_value) {
					con_value = agreements;
					con++;
				}
				//If we reach the connectivity limit, then exit the loop early:
				if (con == connectivity) {
					break;
				}
				//If this potential ancestor agrees with the current witness here, then add an edge for it:
				if (reading_support.find(potential_ancestor_id) != reading_support.end()) {
					string potential_ancestor_rdg = reading_support.at(potential_ancestor_id);
					if (ls.path_exists(potential_ancestor_rdg, wit_rdg) && ls.get_path(potential_ancestor_rdg, wit_rdg).weight == 0) {
						//Set the flag indicating that we've found a textual_flow_ancestor:
						textual_flow_ancestor_found = true;
						//Calculate the stability of the textual flow:
						float strength = float(comp.posterior.cardinality() - comp.prior.cardinality()) / float(comp.extant.cardinality());
						//Add an edge to the graph connecting the textual flow ancestor to this witness:
						textual_flow_edge e;
						e.descendant = wit_id;
						e.ancestor = potential_ancestor_id;
						e.type = flow_type::EQUAL;
						e.connectivity = con;
						e.strength = strength;
						edges.push_back(e);
						break;
					}
				}
			}
		}
		//If the witness is lacunose or it does not have a potential ancestor that agrees with it within the connectivity limit,
		//then each potential ancestor with a distinct reading within the connectivity limit is a textual flow ancestor:
		if (!textual_flow_ancestor_found) {
			con = -1;
			con_value = -1;
			list<string> distinct_rdgs = list<string>();
			for (string potential_ancestor_id : potential_ancestor_ids) {
				//Update the connectivity rank if the connectivity value changes:
				genealogical_comparison comp = wit.get_genealogical_comparison_for_witness(potential_ancestor_id);
				int agreements = (int) comp.agreements.cardinality();
				if (agreements != con_value) {
					con_value = agreements;
					con++;
				}
				//If we reach the connectivity limit, then exit the loop early:
				if (con == connectivity) {
					break;
				}
				//If this potential ancestor has a reading we haven't encountered yet, then add an edge for it:
				if (reading_support.find(potential_ancestor_id) != reading_support.end()) {
					string potential_ancestor_rdg = reading_support.at(potential_ancestor_id);
					bool new_rdg = true;
					for (string rdg : distinct_rdgs) {
						if (ls.path_exists(potential_ancestor_rdg, rdg) && ls.get_path(potential_ancestor_rdg, rdg).weight == 0) {
							new_rdg = false;
							break;
						}
					}
					if (new_rdg) {
						distinct_rdgs.push_back(potential_ancestor_rdg);
						//Calculate the stability of the textual flow:
						float strength = float(comp.posterior.cardinality() - comp.prior.cardinality()) / float(comp.extant.cardinality());
						//Add an edge to the graph connecting the textual flow ancestor to this witness:
						textual_flow_edge e;
						e.descendant = wit_id;
						e.ancestor = potential_ancestor_id;
						e.type = wit_rdg.empty() ? flow_type::LOSS : flow_type::CHANGE;
						e.connectivity = con;
						e.strength = strength;
						edges.push_back(e);
					}
				}
			}
		}
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
 * Returns the this textual_flow's list of vertices.
 */
list<textual_flow_vertex> textual_flow::get_vertices() const {
	return vertices;
}

/**
 * Returns the this textual_flow's list of edges.
 */
list<textual_flow_edge> textual_flow::get_edges() const {
	return edges;
}

/**
 * Given an output stream, writes a complete textual flow diagram to output in .dot format.
 * An optional flag indicating whether to format edges to reflect flow strength can also be specified.
 */
void textual_flow::textual_flow_to_dot(ostream & out, bool flow_strengths) {
	//Add the graph first:
	out << "digraph textual_flow {\n";
	//Add a subgraph for the legend:
	out << "\tsubgraph cluster_legend {\n";
	//Add a box node indicating the label of this graph:
	out << "\t\tlabel [shape=plaintext, label=\"" << label << "\\nCon = " << (connectivity == numeric_limits<int>::max() ? "Absolute" : to_string(connectivity)) << "\"];\n";
	out << "\t}\n";
	//Add a subgraph for the plot:
	out << "\tsubgraph cluster_plot {\n";
	//Make its border invisible:
	out << "\t\tstyle=invis;\n";
	//Add a line indicating that nodes have an ellipse shape:
	out << "\t\tnode [shape=ellipse];\n";
	//Add all of the graph nodes, keeping track of their numerical indices:
	unordered_map<string, int> id_to_index = unordered_map<string, int>();
	for (textual_flow_vertex v : vertices) {
		//Map the ID of this vertex to its numerical index:
		string wit_id = v.id;
		string wit_rdg = v.rdg;
		unsigned int i = (unsigned int) id_to_index.size();
		id_to_index[wit_id] = i;
		int wit_ind = id_to_index.at(wit_id);
		//Then add the node:
		out << "\t\t" << wit_ind;
		//Format the node based on its readings list:
		if (wit_rdg.empty()) {
			//The witness is lacunose at this variation unit:
			out << " [label=\"" << wit_id << "\", color=gray, style=dashed]";
		}
		else {
			//The witness has a reading at this variation unit:
			out << " [label=\"" << wit_id << " (" << wit_rdg << ")\"]";
		}
		out << ";\n";
	}
	//Add all of the graph edges, except for secondary graph edges for changes:
	unordered_set<string> processed_destinations = unordered_set<string>();
	for (textual_flow_edge e : edges) {
		//Get the endpoints' IDs:
		string ancestor_id = e.ancestor;
		string descendant_id = e.descendant;
		//If the destination already has an edge drawn to it, then skip this edge:
		if (processed_destinations.find(descendant_id) != processed_destinations.end()) {
			continue;
		}
		//Otherwise, add the destination node's ID to the processed set:
		processed_destinations.insert(descendant_id);
		//Get the indices corresponding to the endpoints' IDs:
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

/**
 * Given an output stream, writes a complete textual flow diagram to output in JavaScript Object Notation (JSON) format.
 */
void textual_flow::textual_flow_to_json(ostream & out) {
	//Open the root object:
    out << "{";
    //Add the metadata fields:
	out << "\"label\":" << "\"" << label << "\"" << ",";
	out << "\"connectivity\":" << connectivity << ",";
	//Add all of the original vertices:
	list<textual_flow_vertex> textual_flow_vertices = list<textual_flow_vertex>(vertices);
	//Add all of the graph edges, except for secondary graph edges for changes:
    unordered_set<string> processed_descendants = unordered_set<string>();
    list<textual_flow_edge> textual_flow_edges = list<textual_flow_edge>();
    for (textual_flow_edge e : edges) {
	    if (processed_descendants.find(e.descendant) != processed_descendants.end()) {
	        continue;
	    }
	    processed_descendants.insert(e.descendant);
	    textual_flow_edges.push_back(e);
	}
    //Open the vertices array:
    out << "\"vertices\":" << "[";
    //Print each vertex as an object:
	unsigned int vertex_num = 0;
	for (textual_flow_vertex v : textual_flow_vertices) {
        //Open the vertex object:
        out << "{";
        //Add its key-value pairs:
        out << "\"id\":" << "\"" << v.id << "\"" << ",";
        out << "\"rdg\":" << "\"" << v.rdg << "\"" << ",";
        //Close the vertex object:
        out << "}";
        //Add a comma if this is not the last vertex:
        if (vertex_num != textual_flow_vertices.size() - 1) {
            out << ",";
        }
		vertex_num++;
	}
	//Close the vertices array:
    out << "]" << ",";
	//Open the edges array:
    out << "\"edges\":" << "[";
    //Print each edge as an object:
	unsigned int edge_num = 0;
	for (textual_flow_edge e : textual_flow_edges) {
        //Open the edge object:
        out << "{";
        //Add its key-value pairs:
        out << "\"ancestor\":" << "\"" << e.ancestor << "\"" << ",";
        out << "\"descendant\":" << "\"" << e.descendant << "\"" << ",";
        out << "\"type\":" << e.type << ",";
        out << "\"connectivity\":" << e.connectivity << ",";
        out << "\"strength\":" << e.strength;
        //Close the edge object:
        out << "}";
        //Add a comma if this is not the last edge:
        if (edge_num != textual_flow_edges.size() - 1) {
            out << ",";
        }
		edge_num++;
	}
	//Close the edges array:
    out << "]";
    //Close the root object:
    out << "}";
	return;
}

/**
 * Given a reading ID and an output stream,
 * writes a coherence in attestations diagram for that reading to output in .dot format.
 * An optional flag indicating whether to format edges to reflect flow strength can also be specified.
 */
void textual_flow::coherence_in_attestations_to_dot(ostream & out, const string & rdg, bool flow_strengths) {
	//Add the graph first:
	out << "digraph textual_flow_diagram {\n";
	//Add a subgraph for the legend:
	out << "\tsubgraph cluster_legend {\n";
	//Add a box node indicating the label of this graph:
	out << "\t\tlabel [shape=plaintext, label=\"" << label << rdg << "\\nCon = " << (connectivity == numeric_limits<int>::max() ? "Absolute" : to_string(connectivity)) << "\"];\n";
	out << "\t}\n";
	//Add a subgraph for the plot:
	out << "\tsubgraph cluster_plot {\n";
	//Make its border invisible:
	out << "\t\tstyle=invis;\n";
	//Add a line indicating that nodes have an ellipse shape:
	out << "\t\tnode [shape=ellipse];\n";
	//Maintain a map of node IDs to numerical indices
	//and a vector of vertex data structures:
	unordered_map<string, int> id_to_index = unordered_map<string, int>();
	vector<textual_flow_vertex> indexed_vertices = vector<textual_flow_vertex>();
	for (textual_flow_vertex v : vertices) {
		//Map the ID of this vertex to its numerical index:
		string wit_id = v.id;
		unsigned int i = (unsigned int) id_to_index.size();
		id_to_index[wit_id] = i;
		indexed_vertices.push_back(v);
	}
	//Now draw the primary set of vertices corresponding to witnesses with the input reading:
	unordered_set<string> primary_set = unordered_set<string>();
	for (textual_flow_vertex v : vertices) {
		//If this witness does not have the specified reading, then skip it:
		string wit_id = v.id;
		string wit_rdg = v.rdg;
		if (wit_rdg != rdg) {
			continue;
		}
		//Otherwise, draw a vertex with its numerical index:
		int wit_ind = id_to_index.at(wit_id);
		//Then add the node:
		out << "\t\t" << wit_ind;
		out << " [label=\"" << wit_id << " (" << wit_rdg << ")\"]";
		out << ";\n";
		primary_set.insert(wit_id);
	}
	//Then add a secondary set of vertices for the primary ancestors of these witnesses with a different reading,
	//along with edges indicating changes in reading:
	unordered_set<string> secondary_set = unordered_set<string>();
	unordered_set<string> processed_destinations = unordered_set<string>();
	for (textual_flow_edge e : edges) {
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
		//If the destination already has an edge drawn to it, then skip any secondary ancestors:
		if (processed_destinations.find(descendant_id) != processed_destinations.end()) {
			continue;
		}
		//Otherwise, add the destination node's ID to the processed set:
		processed_destinations.insert(descendant_id);
		//If it's new, then add a vertex for it:
		int ancestor_ind = id_to_index.at(ancestor_id);
		textual_flow_vertex v = indexed_vertices[ancestor_ind];
		string ancestor_rdg = v.rdg;
		out << "\t\t" << ancestor_ind;
		out << " [label=\"" << ancestor_id << " (" << ancestor_rdg << ")\", color=blue, style=dashed]";
		out << ";\n";
		secondary_set.insert(ancestor_id);
	}
	//Add all of the graph edges, except for secondary graph edges for changes:
	processed_destinations = unordered_set<string>();
	for (textual_flow_edge e : edges) {
		//Get the endpoints' IDs:
		string ancestor_id = e.ancestor;
		string descendant_id = e.descendant;
		//If the descendant is not in the primary vertex set, then skip this edge:
		if (primary_set.find(descendant_id) == primary_set.end()) {
			continue;
		}
		//If the destination already has an edge drawn to it, then skip this edge:
		if (processed_destinations.find(descendant_id) != processed_destinations.end()) {
			continue;
		}
		//Get the indices of the endpoints:
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
		//Finally, add the destination node's ID to the processed set:
		processed_destinations.insert(descendant_id);
	}
	out << "\t}\n";
	out << "}" << endl;
	return;
}

/**
 * Given a reading ID and an output stream, writes a coherence in attestations textual flow diagram to output in JavaScript Object Notation (JSON) format.
 */
void textual_flow::coherence_in_attestations_to_json(ostream & out, const string & rdg) {
	//Open the root object:
    out << "{";
    //Add the metadata fields:
	out << "\"label\":" << "\"" << label << "\"" << ",";
	out << "\"connectivity\":" << connectivity << ",";
    //Initially filter out any vertices that do not have the given reading:
    list<textual_flow_vertex> coherence_in_attestations_vertices = list<textual_flow_vertex>(vertices);
    coherence_in_attestations_vertices.remove_if([&](const textual_flow_vertex & v) {
		return v.rdg != rdg;
	});
    //Add all of the graph edges ending at one of the remaining vertices, except for secondary graph edges for changes:
    unordered_set<string> witnesses_with_rdg = unordered_set<string>();
    for (textual_flow_vertex v : coherence_in_attestations_vertices) {
        witnesses_with_rdg.insert(v.id);
    }
    list<textual_flow_edge> edges_with_descendants_with_rdg = list<textual_flow_edge>(edges);
    edges_with_descendants_with_rdg.remove_if([&](const textual_flow_edge & e) {
		return witnesses_with_rdg.find(e.descendant) == witnesses_with_rdg.end();
	});
	unordered_set<string> processed_descendants = unordered_set<string>();
	list<textual_flow_edge> distinct_descendant_edges = list<textual_flow_edge>();
    for (textual_flow_edge e : edges_with_descendants_with_rdg) {
	    if (processed_descendants.find(e.descendant) != processed_descendants.end()) {
	        continue;
	    }
	    processed_descendants.insert(e.descendant);
	    distinct_descendant_edges.push_back(e);
	}
	list<textual_flow_edge> coherence_in_attestations_edges = list<textual_flow_edge>(distinct_descendant_edges);
	//Add any ancestors on these edges that do not have the given reading:
	unordered_set<string> ancestors_without_rdg = unordered_set<string>();
	for (textual_flow_edge e : distinct_descendant_edges) {
	    if (witnesses_with_rdg.find(e.ancestor) == witnesses_with_rdg.end()) {
	        ancestors_without_rdg.insert(e.ancestor);
	    }
	}
	for (textual_flow_vertex v : vertices) {
	    if (ancestors_without_rdg.find(v.id) != ancestors_without_rdg.end()) {
	        coherence_in_attestations_vertices.push_back(v);
	    }
	}
	//Open the vertices array:
    out << "\"vertices\":" << "[";
	//Print each vertex as an object:
	unsigned int vertex_num = 0;
	for (textual_flow_vertex v : coherence_in_attestations_vertices) {
        //Open the vertex object:
        out << "{";
        //Add its key-value pairs:
        out << "\"id\":" << "\"" << v.id << "\"" << ",";
        out << "\"rdg\":" << "\"" << v.rdg << "\"" << ",";
        //Close the vertex object:
        out << "}";
        //Add a comma if this is not the last vertex:
        if (vertex_num != coherence_in_attestations_vertices.size() - 1) {
            out << ",";
        }
		vertex_num++;
	}
	//Close the vertices array:
    out << "]" << ",";
	//Open the edges array:
    out << "\"edges\":" << "[";
    //Print each edge as an object:
	unsigned int edge_num = 0;
	for (textual_flow_edge e : coherence_in_attestations_edges) {
        //Open the edge object:
        out << "{";
        //Add its key-value pairs:
        out << "\"ancestor\":" << "\"" << e.ancestor << "\"" << ",";
        out << "\"descendant\":" << "\"" << e.descendant << "\"" << ",";
        out << "\"type\":" << e.type << ",";
        out << "\"connectivity\":" << e.connectivity << ",";
        out << "\"strength\":" << e.strength;
        //Close the edge object:
        out << "}";
        //Add a comma if this is not the last edge:
        if (edge_num != coherence_in_attestations_edges.size() - 1) {
            out << ",";
        }
		edge_num++;
	}
	//Close the edges array:
    out << "]";
    //Close the root object:
    out << "}";
	return;
}

/**
 * Given an output stream, writes a coherence in variant passages diagram to output in .dot format.
 * An optional flag indicating whether to format edges to reflect flow strength can also be specified.
 */
void textual_flow::coherence_in_variant_passages_to_dot(ostream & out, bool flow_strengths) {
	//Add the graph first:
	out << "digraph textual_flow_diagram {\n";
	//Add a subgraph for the legend:
	out << "\tsubgraph cluster_legend {\n";
	//Add a box node indicating the label of this graph:
	out << "\t\tlabel [shape=plaintext, label=\"" << label << "\\nCon = " << (connectivity == numeric_limits<int>::max() ? "Absolute" : to_string(connectivity)) << "\"];\n";
	out << "\t}\n";
	//Add a subgraph for the plot:
	out << "\tsubgraph cluster_plot {\n";
	//Make its border invisible:
	out << "\t\tstyle=invis;\n";
	//Add a line indicating that nodes have an ellipse shape:
	out << "\t\tnode [shape=ellipse];\n";
	//Maintain a map of node IDs to numerical indices
	//and a vector of vertex data structures:
	unordered_map<string, int> id_to_index = unordered_map<string, int>();
	for (textual_flow_vertex v : vertices) {
		//Map the ID of this vertex to its numerical index:
		string wit_id = v.id;
		unsigned int i = (unsigned int) id_to_index.size();
		id_to_index[wit_id] = i;
	}
	//Maintain a map of support lists for each reading:
	map<string, list<string>> clusters = map<string, list<string>>();
	for (string rdg : readings) {
		clusters[rdg] = list<string>();
	}
	for (textual_flow_vertex v : vertices) {
		string wit_id = v.id;
		string wit_rdg = v.rdg;
		clusters[wit_rdg].push_back(wit_id);
	}
	//Maintain a set of IDs for nodes between which there exists an edge of flow type CHANGE:
	unordered_set<string> change_wit_ids = unordered_set<string>();
	for (textual_flow_edge e : edges) {
		if (e.type == flow_type::CHANGE) {
			change_wit_ids.insert(e.ancestor);
			change_wit_ids.insert(e.descendant);
		}
	}
	//Add a cluster for each reading, including all of the nodes it contains:
	for (string rdg : readings) {
		list<string> cluster = clusters.at(rdg);
		out << "\t\tsubgraph cluster_" << rdg << " {\n";
		out << "\t\t\tlabeljust=\"c\";\n";
		out << "\t\t\tlabel=\"" << rdg << "\";\n";
		out << "\t\t\tstyle=solid;\n";
		for (string wit_id : cluster) {
			//If this witness is not at either end of a CHANGE flow edge, then skip it:
			if (change_wit_ids.find(wit_id) == change_wit_ids.end()) {
				continue;
			}
			//Otherwise, add a vertex for it:
			int wit_ind = id_to_index.at(wit_id);
			out << "\t\t\t" << wit_ind;
			out << " [label=\"" << wit_id << "\"]";
			out << ";\n";
		}
		out << "\t\t}\n";
	}
	//Finally, add the "CHANGE" edges:
	for (textual_flow_edge e : edges) {
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

/**
 * Given an output stream, writes a coherence in variant passages textual flow diagram to output in JavaScript Object Notation (JSON) format.
 */
void textual_flow::coherence_in_variant_passages_to_json(ostream & out) {
	//Open the root object:
    out << "{";
    //Add the metadata fields:
	out << "\"label\":" << "\"" << label << "\"" << ",";
	out << "\"connectivity\":" << connectivity << ",";
	//Start with an initially empty vertex list:
	list<textual_flow_vertex> coherence_in_variant_passages_vertices = list<textual_flow_vertex>();
	//Add all of the graph edges representing changes in reading:
    list<textual_flow_edge> coherence_in_variant_passages_edges = list<textual_flow_edge>(edges);
    coherence_in_variant_passages_edges.remove_if([&](const textual_flow_edge & e) {
        return e.type != flow_type::CHANGE;
    });
    //Then add all vertices at either endpoint of these edges:
    unordered_set<string> processed_vertex_ids = unordered_set<string>();
    for (textual_flow_edge e : coherence_in_variant_passages_edges) {
        processed_vertex_ids.insert(e.ancestor);
        processed_vertex_ids.insert(e.descendant);
    }
    coherence_in_variant_passages_vertices = list<textual_flow_vertex>(vertices);
    coherence_in_variant_passages_vertices.remove_if([&](const textual_flow_vertex & v) {
        return processed_vertex_ids.find(v.id) == processed_vertex_ids.end();
    });
    //Open the vertices array:
    out << "\"vertices\":" << "[";
    //Print each vertex as an object:
	unsigned int vertex_num = 0;
	for (textual_flow_vertex v : coherence_in_variant_passages_vertices) {
        //Open the vertex object:
        out << "{";
        //Add its key-value pairs:
        out << "\"id\":" << "\"" << v.id << "\"" << ",";
        out << "\"rdg\":" << "\"" << v.rdg << "\"" << ",";
        //Close the vertex object:
        out << "}";
        //Add a comma if this is not the last vertex:
        if (vertex_num != coherence_in_variant_passages_vertices.size() - 1) {
            out << ",";
        }
		vertex_num++;
	}
	//Close the vertices array:
    out << "]" << ",";
	//Open the edges array:
    out << "\"edges\":" << "[";
    //Print each edge as an object:
	unsigned int edge_num = 0;
	for (textual_flow_edge e : coherence_in_variant_passages_edges) {
        //Open the edge object:
        out << "{";
        //Add its key-value pairs:
        out << "\"ancestor\":" << "\"" << e.ancestor << "\"" << ",";
        out << "\"descendant\":" << "\"" << e.descendant << "\"" << ",";
        out << "\"type\":" << e.type << ",";
        out << "\"connectivity\":" << e.connectivity << ",";
        out << "\"strength\":" << e.strength;
        //Close the edge object:
        out << "}";
        //Add a comma if this is not the last edge:
        if (edge_num != coherence_in_variant_passages_edges.size() - 1) {
            out << ",";
        }
		edge_num++;
	}
	//Close the edges array:
    out << "]";
    //Close the root object:
    out << "}";
	return;
}
