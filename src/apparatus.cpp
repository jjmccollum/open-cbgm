/*
 * Apparatus.cpp
 *
 *  Created on: Oct 24, 2019
 *      Author: jjmccollum
 */

#include <string>
#include <vector>
#include <unordered_set>
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
 * Constructs an apparatus from a <TEI/> XML element and a set of strings indicating reading types that should be treated as substantive is also expected.
 */
apparatus::apparatus(const pugi::xml_node & xml, const unordered_set<string> & substantive_reading_types) {
	//Populate the set of witness IDs first:
	list_wit = unordered_set<string>();
	for (pugi::xpath_node wit_path : xml.select_nodes("teiHeader/sourceDesc/listWit/witness")) {
		pugi::xml_node wit = wit_path.node();
		string wit_id = wit.attribute("xml:id") ? wit.attribute("xml:id").value() : (wit.attribute("id") ? wit.attribute("id").value() : (wit.attribute("n") ? wit.attribute("n").value() : ""));
		list_wit.insert(wit_id);
	}
	//Then parse the variation units:
	variation_units = vector<variation_unit>();
	for (pugi::xpath_node app_path : xml.select_nodes("descendant::app")) {
		pugi::xml_node app = app_path.node();
		variation_unit vu = variation_unit(variation_units.size(), app, substantive_reading_types);
		variation_units.push_back(vu);
	}
}

/**
 * Default destructor.
 */
apparatus::~apparatus() {

}

/**
 * Returns this apparatus's set of witness IDs.
 */
unordered_set<string> apparatus::get_list_wit() const {
	return list_wit;
}

/**
 * Returns this apparatus's vector of variation_units.
 */
vector<variation_unit> apparatus::get_variation_units() const {
	return variation_units;
}
