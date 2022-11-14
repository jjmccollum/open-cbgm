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

#include "pugixml.hpp"
#include "variation_unit.h"

class apparatus {
private:
	std::list<std::string> list_wit;
	std::vector<variation_unit> variation_units;
public:
	apparatus();
	apparatus(const pugi::xml_node & xml, bool merge_splits, const std::set<std::string> & trivial_reading_types, const std::set<std::string> & dropped_reading_types, const std::list<std::string> & ignored_suffixes);
	virtual ~apparatus();
	void set_list_wit(const std::list<std::string> & _list_wit);
	std::list<std::string> get_list_wit() const;
	std::vector<variation_unit> get_variation_units() const;
	int get_extant_passages_for_witness(const std::string & wit_id) const;
};

#endif /* APPARATUS_H */
