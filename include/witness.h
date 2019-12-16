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
	witness(const string & witness_id, const apparatus & app);
	witness(const string & witness_id, const unordered_set<string> & list_wit, const apparatus & app);
	virtual ~witness();
	string get_id() const;
	unordered_map<string, Roaring> get_agreements_by_witness() const;
	unordered_map<string, Roaring> get_explained_readings_by_witness() const;
	Roaring get_agreements_for_witness(const string & other_id) const;
	Roaring get_explained_readings_for_witness(const string & other_id) const;
	bool pregenealogical_comp(const witness & w1, const witness & w2);
	list<string> get_potential_ancestor_ids() const;
	void set_potential_ancestor_ids(const unordered_map<string, witness> & witnesses_by_id);
	list<string> get_global_stemma_ancestors() const;
	void set_global_stemma_ancestors();
};

#endif /* WITNESS_H */
