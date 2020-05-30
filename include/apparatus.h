/*
 * apparatus.h
 *
 *  Created on: Oct 24, 2019
 *      Author: jjmccollum
 */

#ifndef APPARATUS_H
#define APPARATUS_H

#include <string>
#include <list>
#include <vector>
#include <set>

#include "pugixml.h"
#include "variation_unit.h"

using namespace std;

class apparatus {
private:
	list<string> list_wit;
	vector<variation_unit> variation_units;
public:
	apparatus();
	apparatus(const pugi::xml_node & xml, bool merge_splits, const set<string> & trivial_reading_types, const set<string> & dropped_reading_types);
	virtual ~apparatus();
	list<string> get_list_wit() const;
	vector<variation_unit> get_variation_units() const;
	int get_extant_passages_for_witness(const string & wit_id) const;
};

#endif /* APPARATUS_H */
