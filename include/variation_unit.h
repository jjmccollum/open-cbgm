/*
 * variation_unit.h
 *
 *  Created on: Oct 24, 2019
 *      Author: jjmccollum
 */

#ifndef VARIATION_UNIT_H
#define VARIATION_UNIT_H

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <unordered_map>
#include <limits>

#include "pugixml.h"
#include "local_stemma.h"


using namespace std;

class variation_unit {
private:
	string id;
	string label;
	list<string> readings;
	unordered_map<string, list<string>> reading_support;
	int connectivity = numeric_limits<int>::max(); //absolute connectivity by default
	local_stemma stemma;
public:
	variation_unit();
	variation_unit(const pugi::xml_node & xml, const set<string> & substantive_reading_types);
	virtual ~variation_unit();
	string get_id() const;
	string get_label() const;
	list<string> get_readings() const;
	unordered_map<string, list<string>> get_reading_support() const;
	int get_connectivity() const;
	local_stemma get_local_stemma() const;
};

#endif /* VARIATION_UNIT_H */
