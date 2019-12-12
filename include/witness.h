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
#include <unordered_set>

#include "roaring.hh"
#include "apparatus.h"

using namespace std;

class witness {
private:
	string id;
	unordered_map<string, Roaring> agreements_by_witness;
	unordered_map<string, Roaring> explained_readings_by_witness;
	list<string> potential_ancestor_ids;
	list<string> global_stemma_ancestor_ids;
public:
	witness();
	witness(string witness_id, apparatus app);
	witness(string witness_id, unordered_set<string> list_wit, apparatus app);
	virtual ~witness();
	string get_id();
	unordered_map<string, Roaring> get_agreements_by_witness();
	unordered_map<string, Roaring> get_explained_readings_by_witness();
	Roaring get_agreements_for_witness(string other_id);
	Roaring get_explained_readings_for_witness(string other_id);
	bool pregenealogical_comp(witness & w1, witness & w2);
	list<string> get_potential_ancestor_ids();
	void set_potential_ancestor_ids(list<witness> wits);
	list<string> get_global_stemma_ancestors();
	void set_global_stemma_ancestors();
};

#endif /* WITNESS_H */
