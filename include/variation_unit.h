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

#include "pugixml.hpp"
#include "local_stemma.h"

class variation_unit {
private:
	std::string id;
	std::string label;
	std::list<std::string> readings;
	std::unordered_map<std::string, std::string> reading_support;
	int connectivity = std::numeric_limits<int>::max(); //absolute connectivity by default
	local_stemma stemma;
public:
	variation_unit();
	variation_unit(const pugi::xml_node & xml, bool merge_splits, const std::set<std::string> & trivial_reading_types, const std::set<std::string> & dropped_reading_types);
	variation_unit(const std::string & _id, const std::string & _label, const std::list<std::string> & _readings, const std::unordered_map<std::string, std::string> & _reading_support, int _connectivity, const local_stemma & _stemma);
	virtual ~variation_unit();
	std::string get_id() const;
	std::string get_label() const;
	std::list<std::string> get_readings() const;
	std::unordered_map<std::string, std::string> get_reading_support() const;
	int get_connectivity() const;
	local_stemma get_local_stemma() const;
};

#endif /* VARIATION_UNIT_H */
