/*
 * apparatus.h
 *
 *  Created on: Oct 24, 2019
 *      Author: jjmccollum
 */

#ifndef APPARATUS_H
#define APPARATUS_H

#include <string>
#include <unordered_set>

#include "pugixml.h"
#include "variation_unit.h"

using namespace std;

class apparatus {
private:
	unordered_set<string> list_wit;
	list<variation_unit> variation_units;
public:
	apparatus();
	apparatus(const pugi::xml_node xml);
	virtual ~apparatus();
	int size();
	unordered_set<string> get_list_wit();
	list<variation_unit> get_variation_units();
	/*int get_extant_readings_count_for_witness_id(string witness_id);*/
};

#endif /* APPARATUS_H */
