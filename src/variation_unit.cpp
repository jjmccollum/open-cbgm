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
	//Populate the label, if one is specified (if not, use the ID):
	label = (xml.child("label") && xml.child("label").text()) ? xml.child("label").text().get() : id;
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
