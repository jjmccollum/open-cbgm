/*
 * Apparatus.cpp
 *
 *  Created on: Oct 24, 2019
 *      Author: jjmccollum
 */

#include <string>
#include <list>
#include <unordered_set>
#include <unordered_map>

#include "pugixml.h"
#include "apparatus.h"
#include "variation_unit.h"

using namespace std;

apparatus::apparatus() {
	/**
	 * Default constructor.
	 */
}

apparatus::apparatus(const pugi::xml_node xml) {
	/**
	 * Constructs an apparatus from a <TEI/> XML element.
	 */
	//Populate the set of witness IDs first:
	list_wit = unordered_set<string>();
	for (pugi::xpath_node wit_path : xml.select_nodes("teiHeader/sourceDesc/listWit/witness")) {
		pugi::xml_node wit = wit_path.node();
		string wit_id = wit.attribute("xml:id") ? wit.attribute("xml:id").value() : (wit.attribute("id") ? wit.attribute("id").value() : (wit.attribute("n") ? wit.attribute("n").value() : ""));
		list_wit.insert(wit_id);
	}
	//Then parse the variation units:
	variation_units = list<variation_unit>();
	for (pugi::xpath_node app_path : xml.select_nodes("descendant::app")) {
		pugi::xml_node app = app_path.node();
		variation_unit vu = variation_unit(variation_units.size(), app);
		variation_units.push_back(vu);
	}
}

apparatus::~apparatus() {

}

/**
 * Returns the number of variation_units in this apparatus.
 */
int apparatus::size() {
	return variation_units.size();
}

/**
 * Returns this apparatus's set of witness IDs.
 */
unordered_set<string> apparatus::get_list_wit() {
	return list_wit;
}

/**
 * Returns this apparatus's list of variation_units.
 */
list<variation_unit> apparatus::get_variation_units() {
	return variation_units;
}

/**
 * Returns the number of variation units at which the witness with the given ID is extant.
 */
/*
int apparatus::get_extant_readings_count_for_witness_id(string wit_id) {
	int extant_readings_count = 0;
	for (variation_unit vu : variation_units) {
		unordered_map<string, list<int>> reading_support = vu.get_reading_support();
		if (reading_support.find(wit_id) != reading_support.end()) {
			extant_readings_count++;
		}
	}
	return extant_readings_count;
}
*/
