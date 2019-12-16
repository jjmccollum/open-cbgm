/*
 * apparatus.h
 *
 *  Created on: Oct 24, 2019
 *      Author: jjmccollum
 */

#ifndef APPARATUS_H
#define APPARATUS_H

#include <string>
#include <vector>
#include <unordered_set>

#include "pugixml.h"
#include "variation_unit.h"

using namespace std;

class apparatus {
private:
	unordered_set<string> list_wit;
	vector<variation_unit> variation_units;
public:
	apparatus();
	apparatus(const pugi::xml_node & xml, const unordered_set<string> & substantive_reading_types);
	virtual ~apparatus();
	unordered_set<string> get_list_wit() const;
	vector<variation_unit> get_variation_units() const;
};

#endif /* APPARATUS_H */
