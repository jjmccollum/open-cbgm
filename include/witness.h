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
#include "set_cover_solver.h"

using namespace std;

 /**
 * Data structure representing a set of genealogical comparisons to a secondary witness relative to a primary witness.
 */
struct genealogical_comparison {
	string primary_wit; //ID of the primary witness
	string secondary_wit; //ID of the secondary witness
	Roaring extant; //passages where both witnesses are extant
	Roaring agreements; //passages where both witnesses agree
	Roaring prior; //passages where the primary witness has a prior reading
	Roaring posterior; //passages where the primary witness has a posterior reading
	Roaring norel; //passages where both witnesses' readings are known to have no directed relationship
	Roaring unclear; //passages where both witnesses' readings have an unknown relationship
	Roaring explained; //passages where the primary witness has a reading explained by that of the secondary witness
	float cost; //genealogical cost of relationship
};

class witness {
private:
	string id;
	unordered_map<string, genealogical_comparison> genealogical_comparisons;
	list<string> potential_ancestor_ids;
	list<string> stemmatic_ancestor_ids;
public:
	witness();
	witness(const string & _id, const apparatus & app, bool classic=false);
	witness(const string & _id, const list<genealogical_comparison> & _genealogical_comparisons);
	virtual ~witness();
	string get_id() const;
	unordered_map<string, genealogical_comparison> get_genealogical_comparisons() const;
	genealogical_comparison get_genealogical_comparison_for_witness(const string & other_id) const;
	list<string> get_potential_ancestor_ids() const;
	list<set_cover_solution> get_substemmata(float ub=0) const;
	void set_stemmatic_ancestor_ids(const list<string> & witnesses);
	list<string> get_stemmatic_ancestor_ids() const;
};

#endif /* WITNESS_H */
