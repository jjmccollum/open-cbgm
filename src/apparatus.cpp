/*
 * apparatus.cpp
 *
 *  Created on: Oct 24, 2019
 *      Author: jjmccollum
 */

#include <string>
#include <list>
#include <vector>
#include <set> //use ordered set for witnesses here so we can iterate through them in order
#include <unordered_map>

#include "pugixml.h"
#include "apparatus.h"
#include "variation_unit.h"

using namespace std;

/**
 * Default constructor.
 */
apparatus::apparatus() {

}

/**
 * Constructs an apparatus from a <TEI/> XML element.
 * A boolean flag indicating whether or not to merge split readings
 * and sets of strings indicating reading types that should be dropped or treated as trivial are also expected.
 */
apparatus::apparatus(const pugi::xml_node & xml, bool merge_splits, const set<string> & trivial_reading_types, const set<string> & dropped_reading_types) {
	//Parse the variation units:
	variation_units = vector<variation_unit>();
	for (pugi::xpath_node app_path : xml.select_nodes("descendant::app")) {
		pugi::xml_node app = app_path.node();
		variation_unit vu = variation_unit(app, merge_splits, trivial_reading_types, dropped_reading_types);
		variation_units.push_back(vu);
	}
	//Then populate the list of witness IDs:
	list_wit = list<string>();
	//Check if the XML file contains a witness list under its TEI header:
	if (xml.select_node("teiHeader/sourceDesc/listWit")) {
		//If so, then copy from the <witness/> elements directly:
		for (pugi::xpath_node wit_path : xml.select_nodes("teiHeader/sourceDesc/listWit/witness")) {
			pugi::xml_node wit = wit_path.node();
			string wit_id = wit.attribute("xml:id") ? wit.attribute("xml:id").value() : (wit.attribute("id") ? wit.attribute("id").value() : (wit.attribute("n") ? wit.attribute("n").value() : ""));
			list_wit.push_back(wit_id);
		}
	} else {
		//If not (as is the case for output from the ITSEE Collation Editor), 
		//then gather all distinct witness IDs from the variation units directly
		//(note that they will added in the order they appear):
		set<string> distinct_wits = set<string>();
		for (variation_unit vu : variation_units) {
			unordered_map<string, string> reading_support = vu.get_reading_support();
			for (pair<string, string> kv : reading_support) {
				string wit_id = string(kv.first);
				if (distinct_wits.find(wit_id) == distinct_wits.end()) {
					list_wit.push_back(wit_id);
					distinct_wits.insert(wit_id);
				}
			}
		}
	}
}

/**
 * Default destructor.
 */
apparatus::~apparatus() {

}

/**
 * Returns this apparatus's list of witness IDs.
 */
list<string> apparatus::get_list_wit() const {
	return list_wit;
}

/**
 * Returns this apparatus's vector of variation_units.
 */
vector<variation_unit> apparatus::get_variation_units() const {
	return variation_units;
}

/**
 * Returns the number of passages at which the witness with the given ID is extant.
 */
int apparatus::get_extant_passages_for_witness(const string & wit_id) const {
	int extant_passages = 0;
	for (variation_unit vu : variation_units) {
		unordered_map<string, string> reading_support = vu.get_reading_support();
		if (reading_support.find(wit_id) != reading_support.end()) {
			extant_passages++;
		}
	}
	return extant_passages;
}
