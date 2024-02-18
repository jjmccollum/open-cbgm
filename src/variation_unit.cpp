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
#include <unordered_set> //for large sets of witness sigla
#include <algorithm>
#include <limits>

#include "pugixml.hpp"
#include "roaring.hh"
#include "witness.h"
#include "variation_unit.h"
#include "local_stemma.h"

using namespace std;
using namespace pugi;

/**
 * Default constructor.
 */
variation_unit::variation_unit() {

}

/**
 * Constructs a variation unit from an <app/> XML element.
 * A boolean flag indicating whether or not to merge split readings
 * and sets of strings indicating reading types that should be dropped or treated as trivial are also expected.
 * A list of suffixes to ignore in witness sigla and an unordered set of base witness sigla are also expected.
 */
variation_unit::variation_unit(const xml_node & xml, bool merge_splits, const set<string> & trivial_reading_types, const set<string> & dropped_reading_types, const list<string> & ignored_suffixes, const unordered_set<string> & base_sigla) {
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
	xpath_node label_path = xml.select_node("note/label");
	if (label_path) {
		xml_node label_node = label_path.node();
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
	//Proceed for each <rdg/> or <witDetail/> element:
	for (xml_node rdg : xml.children()) {
		if (strcmp(rdg.name(), "rdg") != 0 && strcmp(rdg.name(), "witDetail") != 0) {
			continue;
		}
		//Get the reading's ID and its text:
		string rdg_id = rdg.attribute("xml:id") ? rdg.attribute("xml:id").value() : (rdg.attribute("id") ? rdg.attribute("id").value() : (rdg.attribute("n") ? rdg.attribute("n").value() : ""));
    	string rdg_text = rdg.text() ? rdg.text().get() : "";
		//Populate its set of reading types:
		set<string> rdg_types = set<string>();
		if (rdg.attribute("type")) {
			const string type_string = rdg.attribute("type").value();
			string delim = " ";
			size_t start = 0;
    		size_t end = type_string.find(delim);
			while (end != string::npos) {
				rdg_types.insert(type_string.substr(start, end - start));
				start = end + delim.size();
				end = type_string.find(delim, start);
			}
			rdg_types.insert(type_string.substr(start, end)); //add the last token
		}
		//If it has any of the dropped reading types, then do not process this reading:
		if (!rdg_types.empty()) {
		    bool is_dropped = false;
			for (string rdg_type : rdg_types) {
				if (dropped_reading_types.find(rdg_type) != dropped_reading_types.end()) {
				    is_dropped = true;
					break;
				}
			}
			if (is_dropped) {
			    dropped_readings.insert(rdg_id);
			    continue;
			}
		}
		//Otherwise, add it to the map of reading types by reading:
		reading_types_by_reading[rdg_id] = rdg_types;
		//Add the reading ID to the list:
		readings.push_back(rdg_id);
		//If the reading has a non-empty wit attribute, then split its contents into a list of witness IDs:
		list<string> wits = list<string>();
		if (rdg.attribute("wit") && strcmp(rdg.attribute("wit").value(), "") != 0) {
			const string wit_string = rdg.attribute("wit").value();
			string delim = " ";
			size_t start = 0;
    		size_t end = wit_string.find(delim);
			string wit = ""; //placeholder for extracted tokens
			while (end != string::npos) {
				wit = wit_string.substr(start, end - start);
				//Strip any ignored suffixes as necessary:
				wit = get_base_siglum(wit, ignored_suffixes, base_sigla);
				//If the resulting siglum is a base siglum, then add it to the list:
				if (base_sigla.find(wit) != base_sigla.end()) {
					wits.push_back(wit);
				}
				start = end + delim.size();
				end = wit_string.find(delim, start);
			}
			wit = wit_string.substr(start, end - start);
			//Strip any ignored suffixes as necessary:
			wit = get_base_siglum(wit, ignored_suffixes, base_sigla);
			//If the resulting siglum is a base siglum, then add it to the list:
			if (base_sigla.find(wit) != base_sigla.end()) {
				wits.push_back(wit);
			}
		}
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
		//A reading is considered trivial if all of its reading types are trivial reading types:
		if (rdg_types.empty()) {
			continue;
		}
		bool is_trivial = true;
		for (string rdg_type : rdg_types) {
			if (trivial_reading_types.find(rdg_type) == trivial_reading_types.end()) {
				is_trivial = false;
				break;
			}
		}
		if (is_trivial) {
			trivial_readings.insert(rdg_id);
		}
	}
	//Set the connectivity value, using MAX_INT as a default for absolute connectivity:
	connectivity = numeric_limits<int>::max();
	//If there is a connectivity feature with a <numeric> child that has a "value" attribute that parses to an int, then use that value:
	xpath_node numeric_path = xml.select_node("note/fs/f[@name=\"connectivity\"]/numeric");
	if (numeric_path) {
		xml_node numeric = numeric_path.node();
		if (numeric.attribute("value") && numeric.attribute("value").as_int() > 0) {
			connectivity = numeric.attribute("value").as_int();
		}
	}
	//If there is a <graph/> element, then it contains the local stemma for this variation unit:
	xpath_node stemma_path = xml.select_node("note/graph");
	if (stemma_path) {
		xml_node stemma_node = stemma_path.node();
		stemma = local_stemma(stemma_node, id, label, split_pairs, trivial_readings, dropped_readings);
	}
}

/**
 * Constructs a variation unit using values populated from the genealogical cache.
 */
variation_unit::variation_unit(const string & _id, const string & _label, const list<string> & _readings, const unordered_map<string, string> & _reading_support, int _connectivity, const local_stemma & _stemma) {
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

/**
 * Returns the longest prefix of the given witness siglum string corresponding to a base siglum in the given set, stripping it of all suffixes in the given list as necessary.
 * If none of the specified suffixes in the list can be found in the string, then an empty string is returned.
 */
string variation_unit::get_base_siglum(const string & wit_string, const list<string> & ignored_suffixes, const unordered_set<string> & base_sigla) const {
	string base_siglum = string(wit_string);
	//Strip the witness siglum of any initial "#" character, if it has one:
	if (base_siglum.substr(0, 1) == "#") {
		base_siglum.erase(0, 1);
	}
	//If the resulting siglum is already a base siglum, then return it as-is:
	if (base_sigla.find(base_siglum) != base_sigla.end()) {
		return base_siglum;
	}
	//Then continue to strip off prefixes until we can't anymore:
	bool suffix_found = true;
	while (suffix_found) {
		suffix_found = false;
		//Check for and remove any suffix that occurs in the specified list:
		for (string suffix : ignored_suffixes) {
			if (base_siglum.substr(base_siglum.size() - suffix.size(), base_siglum.size()) == suffix) {
				base_siglum.erase(base_siglum.end() - suffix.size(), base_siglum.end());
				suffix_found = true;
				break;
			}
		}
		//If the siglum does not have any suffixes to remove, then exit this loop:
		if (!suffix_found) {
			break;
		}
		//Otherwise, if the resulting string corresponds to a base siglum, then return it,
		//and if not, then continue in the loop:
		if (base_sigla.find(base_siglum) != base_sigla.end()) {
			return base_siglum;
		}
	}
	//If we get here, then no prefix of the witness siglum corresponds to a base siglum; return an empty string:
	return "";
}
