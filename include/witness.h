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
	unordered_set<string> textual_flow_ancestor_ids;
public:
	witness();
	witness(string witness_id, apparatus app);
	witness(string witness_id, string relative_witness_id, apparatus app);
	virtual ~witness();
	string get_id();
	unordered_map<string, Roaring> get_agreements_by_witness();
	unordered_map<string, Roaring> get_explained_readings_by_witness();
	list<string> get_potential_ancestor_ids();
	unordered_set<string> get_textual_flow_ancestor_ids();
	Roaring get_agreements_for_witness(string other_id);
	Roaring get_explained_readings_for_witness(string other_id);
	bool pregenealogical_comp(witness & w1, witness & w2);
	void set_potential_ancestor_ids(list<witness> wits);
	void add_textual_flow_ancestor_id(string ancestor_id);
	unordered_set<string> get_global_stemma_ancestors();
};

#endif /* WITNESS_H */
