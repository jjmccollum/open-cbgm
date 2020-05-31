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
#include <algorithm>
#include <limits>

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
 * A boolean flag indicating whether or not to merge split readings
 * and sets of strings indicating reading types that should be dropped or treated as trivial are also expected.
 */
variation_unit::variation_unit(const pugi::xml_node & xml, bool merge_splits, const set<string> & trivial_reading_types, const set<string> & dropped_reading_types) {
	//Populate the ID, if one is specified:
	id = xml.attribute("xml:id") ? xml.attribute("xml:id").value() : (xml.attribute("id") ? xml.attribute("id").value() : (xml.attribute("n") ? xml.attribute("n").value() : ""));
	//If the "from" and "to" attributes are present 
	//(as they are in the output of the ITSEE Collation Editor),
	//then assume that the ID is a verse index and add the unit indices:
	if (xml.attribute("from") && xml.attribute("to")) {
		string start_unit = xml.attribute("from").value();
		string end_unit = xml.attribute("to").value();
		if (start_unit == end_unit) {
			id += "U" + start_unit;
		} else {
			id += "U" + start_unit + "-" + end_unit;
		}
	}
	//Populate the label, if one is specified (if not, use the ID):
	pugi::xpath_node label_path = xml.select_node("note/label");
	if (label_path) {
		pugi::xml_node label_node = label_path.node();
		label = label_node.text() ? label_node.text().get() : id;
	} else {
		label = id;
	}
	//Populate the list of reading IDs and the witness-to-readings map,
	//keeping track of reading types and splits to merge as necessary:
	readings = list<string>();
	reading_support = unordered_map<string, string>();
	map<string, set<string>> reading_types_by_reading = map<string, set<string>>(); //to identify trivial reading pairs
	map<string, string> reading_to_text = map<string, string>(); //to identify split reading pairs
	map<string, string> text_to_reading = map<string, string>(); //to identify split reading pairs
	set<string> dropped_readings = set<string>();
	//Proceed for each <rdg/> element:
	for (pugi::xml_node rdg : xml.children("rdg")) {
		//Get the reading's ID and its text:
		string rdg_id = rdg.attribute("n").value();
		string rdg_text = rdg.text() ? rdg.text().get() : "";
		//Populate its set of reading types:
		set<string> rdg_types = set<string>();
		const string type_string = rdg.attribute("type") ? rdg.attribute("type").value() : "";
		char * type_chars = new char[type_string.length() + 1];
		strcpy(type_chars, type_string.c_str());
		const char delim[] = " ";
		char * type_token = strtok(type_chars, delim);
		while (type_token) {
			string rdg_type = string(type_token);
			rdg_types.insert(rdg_type);
			type_token = strtok(NULL, delim); //iterate to the next token
		}
		delete [] type_chars;
		delete [] type_token;
		//If its reading types are a subset of the dropped reading types, then do not process this reading:
		if (!rdg_types.empty()) {
			bool is_subset = true;
			for (string rdg_type : rdg_types) {
				if (dropped_reading_types.find(rdg_type) == dropped_reading_types.end()) {
					is_subset = false;
					break;
				}
			}
			if (is_subset) {
				dropped_readings.insert(rdg_id);
				continue;
			}
		}
		//Otherwise, add it to the map of reading types by reading:
		reading_types_by_reading[rdg_id] = rdg_types;
		//Add the reading ID to the list:
		readings.push_back(rdg_id);
		//Split the witness support attribute into a list of witness strings:
		list<string> wits = list<string>();
		const string wit_string = rdg.attribute("wit").value();
		char * wit_chars = new char[wit_string.length() + 1];
		strcpy(wit_chars, wit_string.c_str());
		char * wit_token = strtok(wit_chars, delim); //reuse the space delimiter from before
		while (wit_token) {
			//Strip the reference of any initial "#" character:
			string wit = string(wit_token);
			wit = wit.rfind("#", 0) == 0 ? wit.erase(0, 1) : wit;
			//Strip the reference of any final "*" character:
			wit = wit.rfind("*") == wit.length() - 1 ? wit.erase(wit.length() - 1) : wit;
			//Strip the reference of any final "V" character:
			wit = wit.rfind("V") == wit.length() - 1 ? wit.erase(wit.length() - 1) : wit;
			//Then add the resulting ID to the witnesses set:
			wits.push_back(wit);
			wit_token = strtok(NULL, delim); //iterate to the next token
		}
		delete [] wit_chars;
		delete [] wit_token;
		//Add these witnesses to the reading support map:
		for (string wit : wits) {
			//Add an empty list for each reading we haven't encountered yet:
			reading_support[wit] = rdg_id;
		}
		//Map this reading's ID to its text, for later reference:
		reading_to_text[rdg_id] = rdg_text;
		//If this reading is not a split reading, then map its text to its ID:
		if (rdg_types.find("split") == rdg_types.end()) {
			text_to_reading[rdg_text] = rdg_id;
		}
	}
	//If necessary, populate a set of split reading pairs to connect in the local stemma:
	set<pair<string, string>> split_pairs = set<pair<string, string>>();
	if (merge_splits) {
		for (pair<string, set<string>> kv : reading_types_by_reading) {
			string rdg_id = kv.first;
			set<string> rdg_types = kv.second;
			string rdg_text = reading_to_text.at(rdg_id);
			if (rdg_types.find("split") != rdg_types.end() && text_to_reading.find(rdg_text) != text_to_reading.end()) {
				string matching_rdg_id = text_to_reading.at(rdg_text);
				if (matching_rdg_id != rdg_id) {
					pair<string, string> split_pair = pair<string, string>(rdg_id, matching_rdg_id);
					split_pairs.insert(split_pair);
				}
			}
		}
	}
	//If necessary, populate a set of trivial readings for the local stemma:
	set<string> trivial_readings = set<string>();
	for (pair<string, set<string>> kv : reading_types_by_reading) {
		string rdg_id = kv.first;
		set<string> rdg_types = set<string>(kv.second);
		//Ignore split attestations, as they've already been processed:
		rdg_types.erase("split");
		//A reading is considered trivial if it has reading types that are a subset of the trivial reading types:
		if (rdg_types.empty()) {
			continue;
		}
		bool is_subset = true;
		for (string rdg_type : rdg_types) {
			if (trivial_reading_types.find(rdg_type) == trivial_reading_types.end()) {
				is_subset = false;
				break;
			}
		}
		if (is_subset) {
			trivial_readings.insert(rdg_id);
		}
	}
	//Set the connectivity value, using MAX_INT as a default for absolute connectivity:
	connectivity = numeric_limits<int>::max();
	//If there is a connectivity feature with a <numeric> child that has a "value" attribute that parses to an int, then use that value:
	pugi::xpath_node numeric_path = xml.select_node("note/fs/f[@name=\"connectivity\"]/numeric");
	if (numeric_path) {
		pugi::xml_node numeric = numeric_path.node();
		if (numeric.attribute("value") && numeric.attribute("value").as_int() > 0) {
			connectivity = numeric.attribute("value").as_int();
		}
	}
	//If there is a <graph/> element, then it contains the local stemma for this variation unit:
	pugi::xpath_node stemma_path = xml.select_node("note/graph");
	if (stemma_path) {
		pugi::xml_node stemma_node = stemma_path.node();
		stemma = local_stemma(stemma_node, id, label, split_pairs, trivial_readings, dropped_readings);
	}
}

/**
 * Constructs a variation unit using values populated from the genealogical cache.
 */
variation_unit::variation_unit(const string & _id, const string & _label, const list<string> & _readings, const unordered_map<string, string> & _reading_support, int _connectivity, const local_stemma _stemma) {
	id = _id;
	label = _label;
	readings = _readings;
	reading_support = _reading_support;
	connectivity = _connectivity;
	stemma = _stemma;
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
unordered_map<string, string> variation_unit::get_reading_support() const {
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
