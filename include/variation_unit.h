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
	unordered_map<string, string> reading_support;
	int connectivity = numeric_limits<int>::max(); //absolute connectivity by default
	local_stemma stemma;
public:
	variation_unit();
	variation_unit(const pugi::xml_node & xml, bool merge_splits, const set<string> & trivial_reading_types, const set<string> & dropped_reading_types);
	variation_unit(const string & _id, const string & _label, const list<string> & _readings, const unordered_map<string, string> & _reading_support, int _connectivity, const local_stemma _stemma);
	virtual ~variation_unit();
	string get_id() const;
	string get_label() const;
	list<string> get_readings() const;
	unordered_map<string, string> get_reading_support() const;
	int get_connectivity() const;
	local_stemma get_local_stemma() const;
};

#endif /* VARIATION_UNIT_H */
