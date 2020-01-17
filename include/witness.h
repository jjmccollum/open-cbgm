/*
 * witness.h
 *
 *  Created on: Oct 24, 2019
 *      Author: jjmccollum
 */

#ifndef WITNESS_H
#define WITNESS_H

#include <string>
#include <list>
#include <unordered_map>

#include "roaring.hh"
#include "apparatus.h"

using namespace std;

//Define data structure for genealogical comparison:
struct genealogical_comparison {
	Roaring agreements;
	Roaring explained;
	float cost;
};

class witness {
private:
	string id;
	unordered_map<string, genealogical_comparison> genealogical_comparisons;
	list<string> potential_ancestor_ids;
	list<string> global_stemma_ancestor_ids;
public:
	witness();
	witness(const string & _id, const apparatus & app);
	witness(const string & _id, const list<string> & list_wit, const apparatus & app);
	witness(const string & _id, const unordered_map<string, genealogical_comparison> & _genealogical_comparisons);
	virtual ~witness();
	string get_id() const;
	unordered_map<string, genealogical_comparison> get_genealogical_comparisons() const;
	genealogical_comparison get_genealogical_comparison_for_witness(const string & other_id) const;
	bool potential_ancestor_comp(const witness & w1, const witness & w2) const;
	list<string> get_potential_ancestor_ids() const;
	void set_potential_ancestor_ids(const list<witness> & witnesses);
	list<string> get_global_stemma_ancestor_ids() const;
	void set_global_stemma_ancestor_ids();
};

#endif /* WITNESS_H */
