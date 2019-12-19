/*
 * variation_unit.cpp
 *
 *  Created on: Oct 24, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <cstring>
#include <string>
#include <list>
#include <set>
#include <map> //for small maps keyed by readings
#include <unordered_map> //for large maps keyed by witnesses

#include "pugixml.h"
#include "roaring.hh"
#include "witness.h"
#include "variation_unit.h"
#include "local_stemma.h"

using namespace std;

/**
 * Default constructor.
 */
variation_unit::variation_unit() {

}

/**
 * Constructs a variation unit from an <app/> XML element.
 * A set of strings indicating reading types that should be treated as substantive is also expected.
 */
variation_unit::variation_unit(const pugi::xml_node & xml, const set<string> & distinct_reading_types) {
	//Populate the ID, if one is specified:
	id = xml.attribute("xml:id") ? xml.attribute("xml:id").value() : (xml.attribute("id") ? xml.attribute("id").value() : (xml.attribute("n") ? xml.attribute("n").value() : ""));
	//Populate the label:
	label = (xml.child("label") && xml.child("label").text()) ? xml.child("label").text().get() : "";
	//Populate the list of reading IDs and the witness-to-readings map,
	//and for the local stemma, maintain a map of trivial subvariants to their nearest nontrivial parent readings:
	readings = list<string>();
	reading_support = unordered_map<string, list<string>>();
	map<string, string> trivial_to_significant = map<string, string>();
	map<string, string> text_to_substantive_reading = map<string, string>();
	string last_substantive_rdg = "";
	string last_split_rdg = "";
	string last_orthographic_rdg = "";
	string last_defective_rdg = "";
	string current_rdg = "";
	//Proceed for each <rdg/> element:
	for (pugi::xml_node rdg : xml.children("rdg")) {
		//Get the reading's ID:
		string rdg_id = rdg.attribute("n").value();
		string rdg_text = rdg.text() ? rdg.text().get() : "";
		//The reading can have 2^3 = 8 possible type combinations encode these in an integer:
		int rdg_type_code = 0;
		const string type_string = rdg.attribute("type") ? rdg.attribute("type").value() : "";
		char * type_chars = new char[type_string.length() + 1];
		strcpy(type_chars, type_string.c_str());
		const char delim[] = " ";
		char * type_token = strtok(type_chars, delim);
		while (type_token) {
			//Strip each reference of the "#" character and add the resulting ID to the witnesses set:
			string rdg_type = string(type_token);
			if (rdg_type == "defective") {
				rdg_type_code += 1;
			}
			else if (rdg_type == "orthographic") {
				rdg_type_code += 2;
			}
			else if (rdg_type == "split") {
				rdg_type_code += 4;
			}
			type_token = strtok(NULL, delim); //iterate to the next token
		}
		//Proceed based on the reading type:
		switch (rdg_type_code) {
			case 0: //substantive
				current_rdg = rdg_id;
				//Reset the counters for all sub-variants:
				last_substantive_rdg = rdg_id;
				last_split_rdg = "";
				last_orthographic_rdg = "";
				last_defective_rdg = "";
				//Reset the text to substantive reading map:
				text_to_substantive_reading = map<string, string>();
				//Map this reading's content to its ID (for resolving split readings to their parent substantive readings):
				text_to_substantive_reading[rdg_text] = rdg_id;
				break;
			case 1: //defective form
				if (distinct_reading_types.find("defective") != distinct_reading_types.end()) {
					current_rdg = rdg_id;
					//Map this reading's content to its ID (for resolving split readings to their parent substantive readings):
					text_to_substantive_reading[rdg_text] = rdg_id;
				}
				else {
					current_rdg = last_substantive_rdg;
					//Map this reading's ID to the last substantive reading's ID:
					trivial_to_significant[rdg_id] = last_substantive_rdg;
					//Map this reading's content to the last substantive reading's ID (for resolving split readings to their parent substantive readings):
					text_to_substantive_reading[rdg_text] = last_substantive_rdg;
				}
				//Update the counter for only defective variants:
				last_defective_rdg = rdg_id;
				break;
			case 2: //orthographic subvariant
				if (distinct_reading_types.find("orthographic") != distinct_reading_types.end()) {
					current_rdg = rdg_id;
					//Map this reading's content to its ID (for resolving split readings to their parent substantive readings):
					text_to_substantive_reading[rdg_text] = rdg_id;
				}
				else {
					current_rdg = last_substantive_rdg;
					//Map this reading's ID to the last distinct reading's ID:
					trivial_to_significant[rdg_id] = last_substantive_rdg;
					//Map this reading's content to the last distinct reading's ID (for resolving split readings to their parent substantive readings):
					text_to_substantive_reading[rdg_text] = last_substantive_rdg;
				}
				//Update the counter for orthographic variants, and reset the counter for defective variants:
				last_orthographic_rdg = rdg_id;
				last_defective_rdg = "";
				break;
			case 3: //defective form of an orthographic subvariant
				if (distinct_reading_types.find("defective") != distinct_reading_types.end()) {
					current_rdg = rdg_id;
					//Map this reading's content to its ID (for resolving split readings to their parent substantive readings):
					text_to_substantive_reading[rdg_text] = rdg_id;
				}
				else if (distinct_reading_types.find("orthographic") != distinct_reading_types.end()) {
					current_rdg = last_orthographic_rdg;
					//Map this reading's ID to the last distinct reading's ID:
					trivial_to_significant[rdg_id] = last_orthographic_rdg;
					//Map this reading's content to the last distinct reading's ID (for resolving split readings to their parent substantive readings):
					text_to_substantive_reading[rdg_text] = last_orthographic_rdg;
				}
				else {
					current_rdg = last_substantive_rdg;
					//Map this reading's ID to the last distinct reading's ID:
					trivial_to_significant[rdg_id] = last_substantive_rdg;
					//Map this reading's content to the last distinct reading's ID (for resolving split readings to their parent substantive readings):
					text_to_substantive_reading[rdg_text] = last_substantive_rdg;
				}
				//Update the counter for only defective variants:
				last_defective_rdg = rdg_id;
				break;
			case 4: //split attestation of a substantive reading
				if (distinct_reading_types.find("split") != distinct_reading_types.end()) {
					current_rdg = rdg_id;
				}
				else {
					//Get the non-split reading with the same text:
					current_rdg = text_to_substantive_reading[rdg_text];
					//Map this reading's ID to the last distinct reading's ID:
					trivial_to_significant[rdg_id] = text_to_substantive_reading[rdg_text];
				}
				//Update the counter for split variants, and reset the counter for orthographic and defective variants:
				last_split_rdg = rdg_id;
				last_orthographic_rdg = "";
				last_defective_rdg = "";
				break;
			case 5: //defective form of a split attestation
				if (distinct_reading_types.find("split") != distinct_reading_types.end() && distinct_reading_types.find("defective") != distinct_reading_types.end()) {
					current_rdg = rdg_id;
				}
				else if (distinct_reading_types.find("split") != distinct_reading_types.end()) {
					current_rdg = last_split_rdg;
					//Map this reading's ID to the last distinct reading's ID:
					trivial_to_significant[rdg_id] = last_split_rdg;
				}
				else if (distinct_reading_types.find("defective") != distinct_reading_types.end()) {
					//Check if there is a non-split reading with the same text:
					if (text_to_substantive_reading.find(rdg_text) != text_to_substantive_reading.end()) {
						//If there is, then get the non-split reading with the same text:
						current_rdg = text_to_substantive_reading[rdg_text];
						//Map this reading's ID to the existing reading's ID:
						trivial_to_significant[rdg_id] = text_to_substantive_reading[rdg_text];
					}
					else {
						//Otherwise, this reading should be treated as distinct:
						current_rdg = rdg_id;
						//Map this reading's content to its ID (for resolving split readings to their parent substantive readings):
						text_to_substantive_reading[rdg_text] = rdg_id;
					}
				}
				else {
					//Get the non-split reading with the same text as the last split reading:
					current_rdg = trivial_to_significant[last_split_rdg];
					//Map this reading's ID to that reading's ID:
					trivial_to_significant[rdg_id] = trivial_to_significant[last_split_rdg];
				}
				//Update the counter for only defective variants:
				last_defective_rdg = rdg_id;
				break;
			case 6: //orthographic subvariant of a split attestation
				if (distinct_reading_types.find("split") != distinct_reading_types.end() && distinct_reading_types.find("orthographic") != distinct_reading_types.end()) {
					current_rdg = rdg_id;
				}
				else if (distinct_reading_types.find("split") != distinct_reading_types.end()) {
					current_rdg = last_split_rdg;
					//Map this reading's ID to the last distinct reading's ID:
					trivial_to_significant[rdg_id] = last_split_rdg;
				}
				else if (distinct_reading_types.find("orthographic") != distinct_reading_types.end()) {
					//Check if there is a non-split reading with the same text:
					if (text_to_substantive_reading.find(rdg_text) != text_to_substantive_reading.end()) {
						//If there is, then get the non-split reading with the same text:
						current_rdg = text_to_substantive_reading[rdg_text];
						//Map this reading's ID to the existing reading's ID:
						trivial_to_significant[rdg_id] = text_to_substantive_reading[rdg_text];
					}
					else {
						//Otherwise, this reading should be treated as distinct:
						current_rdg = rdg_id;
						//Map this reading's content to its ID (for resolving split readings to their parent substantive readings):
						text_to_substantive_reading[rdg_text] = rdg_id;
					}
				}
				else {
					//Get the non-split reading with the same text as the last split reading:
					current_rdg = trivial_to_significant[last_split_rdg];
					//Map this reading's ID to that reading's ID:
					trivial_to_significant[rdg_id] = trivial_to_significant[last_split_rdg];
				}
				//Update the counter for orthographic variants, and reset the counter for defective variants:
				last_orthographic_rdg = rdg_id;
				last_defective_rdg = "";
				break;
			case 7: //defective form of an orthographic subvariant of a split attestation
				if (distinct_reading_types.find("split") != distinct_reading_types.end() && distinct_reading_types.find("orthographic") != distinct_reading_types.end() && distinct_reading_types.find("defective") != distinct_reading_types.end()) {
					current_rdg = rdg_id;
				}
				else if (distinct_reading_types.find("split") != distinct_reading_types.end() && distinct_reading_types.find("orthographic") != distinct_reading_types.end()) {
					current_rdg = last_orthographic_rdg;
					//Map this reading's ID to the last distinct reading's ID:
					trivial_to_significant[rdg_id] = last_orthographic_rdg;
				}
				else if (distinct_reading_types.find("split") != distinct_reading_types.end() && distinct_reading_types.find("defective") != distinct_reading_types.end()) {
					current_rdg = last_split_rdg;
					//Map this reading's ID to the last distinct reading's ID:
					trivial_to_significant[rdg_id] = last_split_rdg;
				}
				else if (distinct_reading_types.find("orthographic") != distinct_reading_types.end() && distinct_reading_types.find("defective") != distinct_reading_types.end()) {
					//Check if there is a non-split reading with the same text:
					if (text_to_substantive_reading.find(rdg_text) != text_to_substantive_reading.end()) {
						//If there is, then get the non-split reading with the same text:
						current_rdg = text_to_substantive_reading[rdg_text];
						//Map this reading's ID to the existing reading's ID:
						trivial_to_significant[rdg_id] = text_to_substantive_reading[rdg_text];
					}
					else {
						//Otherwise, this reading should be treated as distinct:
						current_rdg = rdg_id;
						//Map this reading's content to its ID (for resolving split readings to their parent substantive readings):
						text_to_substantive_reading[rdg_text] = rdg_id;
					}
				}
				else if (distinct_reading_types.find("split") != distinct_reading_types.end()) {
					current_rdg = last_split_rdg;
					//Map this reading's ID to the last distinct reading's ID:
					trivial_to_significant[rdg_id] = last_split_rdg;
				}
				else if (distinct_reading_types.find("orthographic") != distinct_reading_types.end()) {
					//Check if there is a non-split reading with the same text as the last orthographic reading:
					if (trivial_to_significant.find(last_orthographic_rdg) != trivial_to_significant.end()) {
						//If there is, then get the non-split reading with its text:
						current_rdg = trivial_to_significant[last_orthographic_rdg];
						//Map this reading's ID to that reading's ID:
						trivial_to_significant[rdg_id] = trivial_to_significant[last_orthographic_rdg];
					}
					else {
						//Otherwise, this reading should be treated as distinct:
						current_rdg = rdg_id;
						//Map this reading's content to its ID (for resolving split readings to their parent substantive readings):
						text_to_substantive_reading[rdg_text] = rdg_id;
					}
				}
				else if (distinct_reading_types.find("defective") != distinct_reading_types.end()) {
					//Check if there is a non-split reading with the same text:
					if (text_to_substantive_reading.find(rdg_text) != text_to_substantive_reading.end()) {
						//If there is, then get the non-split reading with the same text:
						current_rdg = text_to_substantive_reading[rdg_text];
						//Map this reading's ID to the existing reading's ID:
						trivial_to_significant[rdg_id] = text_to_substantive_reading[rdg_text];
					}
					else {
						//Otherwise, this reading should be treated as distinct:
						current_rdg = rdg_id;
						//Map this reading's content to its ID (for resolving split readings to their parent substantive readings):
						text_to_substantive_reading[rdg_text] = rdg_id;
					}
				}
				else {
					//Get the non-split reading corresponding to the last split reading:
					current_rdg = trivial_to_significant[last_split_rdg];
					//Map this reading's ID to the existing reading's ID:
					trivial_to_significant[rdg_id] = trivial_to_significant[last_split_rdg];
				}
				//Update the counter for only defective variants:
				last_defective_rdg = rdg_id;
				break;
			default: //unknown code (this should never happen)
				break;
		}
		//If the raw reading ID is not mapped to an existing reading ID, then add it to the list:
		if (trivial_to_significant.find(rdg_id) == trivial_to_significant.end()) {
			readings.push_back(rdg_id);
		}
		//Now split the witness support attribute into a list of witness strings:
		list<string> wits = list<string>();
		const string wit_string = rdg.attribute("wit").value();
		char * wit_chars = new char[wit_string.length() + 1];
		strcpy(wit_chars, wit_string.c_str());
		char * wit_token = strtok(wit_chars, delim); //reuse the space delimiter from before
		while (wit_token) {
			//Strip each reference of the "#" character and add the resulting ID to the witnesses set:
			string wit = string(wit_token);
			wit = wit.rfind("#", 0) == 0 ? wit.erase(0, 1) : wit;
			wits.push_back(wit);
			wit_token = strtok(NULL, delim); //iterate to the next token
		}
		//Then process each witness with this reading:
		for (string wit : wits) {
			//Add an empty list for each reading we haven't encountered yet:
			if (reading_support.find(wit) == reading_support.end()) {
				reading_support[wit] = list<string>();
			}
			reading_support[wit].push_back(current_rdg);
		}
	}
	//Set the connectivity value:
	pugi::xpath_node numeric_path = xml.select_node("fs/f[@name=\"connectivity\"]/numeric");
	if (numeric_path) {
		pugi::xml_node numeric = numeric_path.node();
		connectivity = numeric.attribute("value") ? numeric.attribute("value").as_int() : connectivity;
	}
	//Initialize the local stemma graph:
	pugi::xml_node stemma_node = xml.child("graph");
	if (stemma_node) {
		//The <graph/> element should contain the local stemma for this variation unit:
		stemma = local_stemma(label, stemma_node, trivial_to_significant);
	}
	//Initialize the textual flow graph as empty:
	graph.vertices = list<textual_flow_vertex>();
	graph.edges = list<textual_flow_edge>();
}

/**
 * Default destructor.
 */
variation_unit::~variation_unit() {

}

/**
 * Returns the ID of this variation_unit.
 */
string variation_unit::get_id() const {
	return id;
}

/**
 * Returns the label of this variation_unit.
 */
string variation_unit::get_label() const {
	return label;
}

/**
 * Returns this variation unit's list of reading IDs.
 */
list<string> variation_unit::get_readings() const {
	return readings;
}

/**
 * Returns the reading support set of this variation_unit.
 */
unordered_map<string, list<string>> variation_unit::get_reading_support() const {
	return reading_support;
}

/**
 * Returns the connectivity of this variation_unit.
 */
int variation_unit::get_connectivity() const {
	return connectivity;
}

/**
 * Returns the local stemma of this variation_unit.
 */
local_stemma variation_unit::get_local_stemma() const {
	return stemma;
}

/**
 * Returns the textual flow diagram of this variation unit.
 */
textual_flow_graph variation_unit::get_textual_flow_diagram() const {
	return graph;
}

/**
 * Given a witness, adds a vertex representing it and an edge representing its relationship to its ancestor to the textual flow diagram graph.
 */
void variation_unit::calculate_textual_flow_for_witness(const witness & w) {
	string wit_id = w.get_id();
	//Check if this witness is lacunose:
	bool extant = (reading_support.find(wit_id) != reading_support.end());
	//Check if it attests to more than one reading:
	bool ambiguous = false;
	if (extant) {
		list<string> readings_for_witness = reading_support[wit_id];
		ambiguous = readings_for_witness.size() > 1;
	}
	//Add a vertex for this witness to the graph:
	textual_flow_vertex v;
	v.id = wit_id;
	v.extant = extant;
	v.ambiguous = ambiguous;
	graph.vertices.push_back(v);
	//If this witness has no potential ancestors (i.e., if it is the Ausgangstext), then we're done:
	if (w.get_potential_ancestor_ids().size() == 0) {
		return;
	}
	//Otherwise, identify this witness's textual flow ancestor for this variation unit:
	string textual_flow_ancestor_id;
	int con = 0;
	flow_type type = flow_type::NONE;
	//If the witness is extant, then attempt to find an ancestor within the connectivity limit that agrees with it:
	if (extant) {
		con = 0;
		list<string> wit_rdgs = reading_support.at(wit_id);
		for (string potential_ancestor_id : w.get_potential_ancestor_ids()) {
			//If we reach the connectivity limit, then exit the loop early:
			if (con == connectivity) {
				break;
			}
			//If this potential ancestor agrees with the current witness here, then we're done:
			bool agree = false;
			if (reading_support.find(potential_ancestor_id) != reading_support.end()) {
				list<string> potential_ancestor_rdgs = reading_support.at(potential_ancestor_id);
				for (string wit_rdg : wit_rdgs) {
					for (string potential_ancestor_rdg : potential_ancestor_rdgs) {
						if (wit_rdg == potential_ancestor_rdg) {
							agree = true;
						}
					}
				}
			}
			if (agree) {
				textual_flow_ancestor_id = potential_ancestor_id;
				type = ambiguous ? flow_type::AMBIGUOUS : flow_type::EQUAL;
				break;
			}
			//Otherwise, continue through the loop:
			con++;
		}
	}
	//If no textual flow ancestor has been found (either because the witness is lacunose or it does not have a close ancestor that agrees with it),
	//then its first extant potential ancestor is its textual flow ancestor:
	if (textual_flow_ancestor_id.empty()) {
		con = 0;
		for (string potential_ancestor_id : w.get_potential_ancestor_ids()) {
			if (reading_support.find(potential_ancestor_id) != reading_support.end()) {
				textual_flow_ancestor_id = potential_ancestor_id;
				break;
			}
			con++;
		}
		//Set the type based on whether or not this witness is extant:
		type = extant ? flow_type::CHANGE : flow_type::LOSS;
	}
	//Add an edge to the graph connecting the current witness to its textual flow ancestor:
	textual_flow_edge e;
	e.descendant = wit_id;
	e.ancestor = textual_flow_ancestor_id;
	e.connectivity = con;
	e.type = type;
	graph.edges.push_back(e);
	return;
}

/**
 * Given a list of witnesses, constructs the textual flow diagram for this variation_unit.
 */
void variation_unit::calculate_textual_flow(const list<witness> & witnesses) {
	graph.vertices = list<textual_flow_vertex>();
	graph.edges = list<textual_flow_edge>();
	//Add a node for each witness:
	for (witness wit : witnesses) {
		calculate_textual_flow_for_witness(wit);
	}
	return;
}

/**
 * Given an output stream, writes this variation_unit's textual flow diagram graph to output in .dot format.
 */
void variation_unit::textual_flow_diagram_to_dot(ostream & out) {
	//Add the graph first:
	out << "digraph textual_flow_diagram {\n";
	//Add a line indicating that nodes do not have any shape:
	out << "\tnode [shape=plaintext];\n";
	//Add a box node indicating the label of this variation_unit:
	out << "\tlabel [shape=box, label=\"" << label << "\\nCon=" << connectivity << "\"];\n";
	//Add all of the graph nodes, keeping track of their numerical indices:
	unordered_map<string, int> id_to_index = unordered_map<string, int>();
	for (textual_flow_vertex v : graph.vertices) {
		//Map the ID of this vertex to its numerical index:
		string wit_id = v.id;
		id_to_index[wit_id] = id_to_index.size();
		int wit_index = id_to_index[wit_id];
		out << "\t" << wit_index;
		//Format the node based on whether the witness is extant or ambiguous here:
		if (v.extant && !v.ambiguous) {
			out << " [label=\"" << wit_id << "\"]";
		}
		else if (v.extant && v.ambiguous) {
			out << " [label=\"" << wit_id << "\", shape=ellipse, peripheries=2]";
		}
		else {
			out << " [label=\"" << wit_id << "\", color=gray, shape=ellipse, style=dashed]";
		}
		out << ";\n";
	}
	//Add all of the graph edges:
	for (textual_flow_edge e : graph.edges) {
		//Get the indices corresponding to the endpoints' IDs:
		int ancestor_index = id_to_index[e.ancestor];
		int descendant_index = id_to_index[e.descendant];
		out << "\t";
		if (e.type == flow_type::AMBIGUOUS) {
			//Ambiguous changes are indicated by double-lined arrows:
			out << ancestor_index << " => " << descendant_index;
		}
		else {
			//All other changes are indicated by single-lined arrows:
			out << ancestor_index << " -> " << descendant_index;
		}
		//Conditionally format the edge:
		out << " [";
		if (e.connectivity > 0) {
			//If the connectivity index is not direct (i.e., 0), then print it in one-based format:
			out << "label=\"" << (e.connectivity + 1) << "\", fontsize=10, ";
		}
		if (e.type == flow_type::CHANGE) {
			//Highlight changes in readings in blue:
			out << "color=blue";
		}
		else if (e.type == flow_type::LOSS) {
			//Highlight losses with dashed gray arrows:
			out << "color=gray, style=dashed";
		}
		else {
			//Equal and ambiguous textual flows are indicated by solid black arrows:
			out << "color=black";
		}
		out << "];\n";
	}
	out << "}" << endl;
	return;
}

/**
 * Given a reading ID and an output stream, writes this variation_unit's textual flow diagram graph for that reading to output in .dot format.
 */
void variation_unit::textual_flow_diagram_for_reading_to_dot(const string & rdg_id, ostream & out) {
	//Add the graph first:
	out << "digraph textual_flow_diagram {\n";
	//Add a line indicating that nodes do not have any shape:
	out << "\tnode [shape=plaintext];\n";
	//Add a box node indicating the label of this variation_unit and the selected reading:
	out << "\tlabel [shape=box, label=\"" << label << rdg_id << "\\nCon=" << connectivity << "\"];\n";
	//Add all of the graph nodes for witnesses with the specified reading, keeping track of their numerical indices:
	unordered_map<string, int> primary_id_to_index = unordered_map<string, int>();
	for (textual_flow_vertex v : graph.vertices) {
		//If this witness does not have the specified reading, then skip it:
		string wit_id = v.id;
		if (reading_support.find(wit_id) == reading_support.end()) {
			continue;
		}
		bool wit_has_reading = false;
		list<string> readings_for_wit = reading_support[wit_id];
		for (string reading : readings_for_wit) {
			if (reading == rdg_id) {
				wit_has_reading = true;
				break;
			}
		}
		if (!wit_has_reading) {
			continue;
		}
		//Otherwise, map its ID to its numerical index:
		primary_id_to_index[wit_id] = primary_id_to_index.size();
		int wit_index = primary_id_to_index[wit_id];
		//Then draw its vertex:
		out << "\t" << wit_index;
		if (v.ambiguous) {
			out << " [label=\"" << wit_id << "\", shape=ellipse, peripheries=2]";
		}
		else {
			out << " [label=\"" << wit_id << "\"]";
		}
		out << ";\n";
	}
	//Add all of the graph nodes for ancestors of these witnesses with a different reading, keeping track of their numerical indices:
	unordered_map<string, int> secondary_id_to_index = unordered_map<string, int>();
	for (textual_flow_edge e : graph.edges) {
		//Get the endpoints' IDs:
		string ancestor_id = e.ancestor;
		string descendant_id = e.descendant;
		//If the descendant is not in the primary vertex set, or the ancestor is, then ignore:
		if (primary_id_to_index.find(descendant_id) == primary_id_to_index.end() || primary_id_to_index.find(ancestor_id) != primary_id_to_index.end()) {
			continue;
		}
		//Otherwise, we have an ancestor with a reading other than the specified one; skip it if we've added its vertex already:
		if (secondary_id_to_index.find(ancestor_id) != secondary_id_to_index.end()) {
			continue;
		}
		//If it's new, then get its reading ID(s):
		list<string> ancestor_readings = reading_support[ancestor_id];
		string ancestor_reading = "";
		for (string rdg : ancestor_readings) {
			ancestor_reading += rdg;
			if (rdg != ancestor_readings.back()) {
				ancestor_reading + " ";
			}
		}
		//Then add a vertex for it to the secondary vertex set:
		secondary_id_to_index[ancestor_id] = primary_id_to_index.size() + secondary_id_to_index.size();
		int wit_index = secondary_id_to_index[ancestor_id];
		out << "\t" << wit_index;
		if (ancestor_readings.size() > 1) {
			//The nearest extant ancestor has an ambiguous reading:
			out << " [label=\"" << ancestor_reading << "\", color=blue, shape=ellipse, peripheries=2, type=dashed]";
		}
		else {
			out << " [label=\"" << ancestor_reading << ": " << ancestor_id << "\", color=blue, shape=ellipse, type=dashed]";
		}
		out << ";\n";
	}
	//Add all of the graph edges:
	for (textual_flow_edge e : graph.edges) {
		//Get the endpoints' IDs:
		string ancestor_id = e.ancestor;
		string descendant_id = e.descendant;
		//If the descendant is not in the primary vertex set, then skip this edge:
		if (primary_id_to_index.find(descendant_id) == primary_id_to_index.end()) {
			continue;
		}
		//Otherwise, get the indices of the endpoints:
		int ancestor_index = primary_id_to_index.find(ancestor_id) != primary_id_to_index.end() ? primary_id_to_index[ancestor_id] : secondary_id_to_index[ancestor_id];
		int descendant_index = primary_id_to_index[descendant_id];
		out << "\t";
		if (e.type == flow_type::AMBIGUOUS) {
			//Ambiguous changes are indicated by double-lined arrows:
			out << ancestor_index << " => " << descendant_index;
		}
		else {
			//All other changes are indicated by single-lined arrows:
			out << ancestor_index << " -> " << descendant_index;
		}
		//Conditionally format the edge:
		out << " [";
		if (e.connectivity > 0) {
			//If the connectivity index is not direct (i.e., 0), then print it in one-based format:
			out << "label=\"" << (e.connectivity + 1) << "\", fontsize=10, ";
		}
		if (e.type == flow_type::CHANGE) {
			//Highlight changes in readings in blue:
			out << "color=blue";
		}
		else if (e.type == flow_type::LOSS) {
			//Highlight losses with dashed gray arrows:
			out << "color=gray, style=dashed";
		} else {
			//Textual flow involving no change is indicated by a black arrow:
			out << "color=black";
		}
		out << "];\n";
	}
	out << "}" << endl;
	return;
}

/**
 * Given an output stream, writes this variation_unit's textual flow diagram graph for only "CHANGE" edges to output in .dot format.
 */
void variation_unit::textual_flow_diagram_for_changes_to_dot(ostream & out) {
	//Add the graph first:
	out << "digraph textual_flow_diagram {\n";
	//Add a line indicating that nodes do not have any shape:
	out << "\tnode [shape=plaintext];\n";
	//Add a box node indicating the label of this variation_unit:
	out << "\tlabel [shape=box, label=\"" << label << "\\nCon=" << connectivity << "\"];\n";
	//Maintain a map of support lists for each reading:
	map<string, list<string>> clusters = map<string, list<string>>();
	for (pair<string, list<string>> kv : reading_support) {
		string wit_id = kv.first;
		list<string> wit_readings = kv.second;
		for (string wit_reading : wit_readings) {
			//Add an empty list of witness IDs for this reading, if it hasn't been encountered yet:
			if (clusters.find(wit_reading) == clusters.end()) {
				clusters[wit_reading] = list<string>();
			}
			clusters[wit_reading].push_back(wit_id);
		}
	}
	//Now map the IDs of the included vertices to numerical indices:
	unordered_map<string, int> id_to_index = unordered_map<string, int>();
	for (textual_flow_edge e : graph.edges) {
		//Only include vertices that are at endpoints of a "CHANGE" edge:
		if (e.type != flow_type::CHANGE) {
			continue;
		}
		string ancestor_id = e.ancestor;
		string descendant_id = e.descendant;
		if (id_to_index.find(ancestor_id) == id_to_index.end()) {
			id_to_index[ancestor_id] = id_to_index.size();
		}
		if (id_to_index.find(descendant_id) == id_to_index.end()) {
			id_to_index[descendant_id] = id_to_index.size();
		}
	}
	//Add a cluster for each reading, including all of the nodes it contains:
	for (pair<string, list<string>> kv : clusters) {
		string rdg_id = kv.first;
		list<string> cluster = kv.second;
		out << "\tsubgraph cluster_" << rdg_id << " {\n";
		out << "\t\tlabeljust=\"c\";\n";
		out << "\t\tlabel=\"" << rdg_id << "\";\n";
		for (string wit_id : cluster) {
			//If this witness's ID is not found in the numerical index map, then skip it:
			if (id_to_index.find(wit_id) == id_to_index.end()) {
				continue;
			}
			//Otherwise, get the numerical index for this witness's vertex:
			int wit_index = id_to_index[wit_id];
			out << "\t\t" << wit_index;
			if (reading_support[wit_id].size() > 1) {
				//The witness is ambiguous:
				out << " [label=\"" << wit_id << "\", shape=ellipse, peripheries=2]";
			}
			else {
				out << " [label=\"" << wit_id << "\"]";
			}
			out << ";\n";
		}
		out << "\t}\n";
	}
	//Finally, add the "CHANGE" edges:
	for (textual_flow_edge e : graph.edges) {
		//Only include vertices that are at endpoints of a "CHANGE" edge:
		if (e.type != flow_type::CHANGE) {
			continue;
		}
		//Get the indices corresponding to the endpoints' IDs:
		int ancestor_index = id_to_index[e.ancestor];
		int descendant_index = id_to_index[e.descendant];
		out << "\t";
		out << ancestor_index << " -> " << descendant_index;
		//Conditionally format the edge:
		out << " [";
		if (e.connectivity > 0) {
			//If the connectivity index is not direct (i.e., 0), then print it in one-based format:
			out << "label=\"" << (e.connectivity + 1) << "\", fontsize=10, ";
		}
		//Highlight changes in readings in blue:
		out << "color=blue";
		out << "];\n";
	}
	out << "}" << endl;
	return;
}
