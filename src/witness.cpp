/*
 * witness.cpp
 *
 *  Created on: Oct 24, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <algorithm>

#include <roaring/roaring.hh>
#include "witness.h"
#include "set_cover_solver.h"
#include "apparatus.h"
#include "variation_unit.h"
#include "local_stemma.h"
#include "witness_comparison_index.h"

using namespace std;
using namespace roaring;

/**
 * Default constructor.
 */
witness::witness() {

}

/**
 * Constructs a witness using its ID and a textual apparatus, 
 * as well as an optional flag indicating whether the "classic" calculation of costs and explained readings should be used.
 */
witness::witness(const string & _id, const apparatus & app, bool classic) {
	//Set its ID:
	id = _id;
	//Now populate the its map of genealogical_comparisons, keyed by witness ID:
	genealogical_comparisons = unordered_map<string, genealogical_comparison>();
	const auto & list_wit = app.list_wit;
	const auto & index = app.comparison_cache->get(app.variation_units);
	const auto & primary_attestations = index.attestations_for(id);
	for (const string & other_id : list_wit) {
		//Initialize a genealogical_comparison data structure for this witness:
		genealogical_comparison comp;
		comp.primary_wit = id;
		comp.secondary_wit = other_id;
		comp.extant = Roaring();
		comp.agreements = Roaring();
		comp.prior = Roaring();
		comp.posterior = Roaring();
		comp.norel = Roaring();
		comp.unclear = Roaring();
		comp.explained = Roaring();
		comp.cost = 0;
		const auto & secondary_attestations = index.attestations_for(other_id);
		auto primary = primary_attestations.begin();
		auto secondary = secondary_attestations.begin();
		// Walk the passages both witnesses attest in collation order.
		while (primary != primary_attestations.end() && secondary != secondary_attestations.end()) {
			if (primary->passage < secondary->passage) {
				++primary;
				continue;
			}
			if (secondary->passage < primary->passage) {
				++secondary;
				continue;
			}
			const auto passage = primary->passage;
			const auto relation = index.relationship(*primary, *secondary, app.variation_units[passage].stemma);
			++primary;
			++secondary;

			comp.extant.add(passage);
			if (relation.agrees) {
				comp.agreements.add(passage);
				comp.explained.add(passage);
				continue;
			}
			if (relation.prior) {
				comp.prior.add(passage);
			}
			if (relation.posterior) {
				comp.posterior.add(passage);
				//The classic criterion is that only a reading equivalent or directly prior to another reading explains it:
				if (!classic || relation.classic_explained) {
					comp.explained.add(passage);
				}
				//The open-cbgm criterion is more relaxed; any equivalent or prior reading explains another,
				//and the cost is equal to the length of the path from the prior reading to the posterior reading:
				if (!classic) {
					comp.cost += relation.weight;
				}
			}
			if (!relation.prior && !relation.posterior) {
				if (relation.common_ancestor) {
					comp.norel.add(passage);
				} else {
					comp.unclear.add(passage);
				}
			}
			//The classic calculation of costs is just 1 in the case of any disagreement:
			if (classic) {
				comp.cost += 1;
			}
		}
		//Add the completed genealogical_comparison to this witness's map:
		genealogical_comparisons[other_id] = comp;
	}
	//Next, populate this witness's list of potential ancestors:
	potential_ancestor_ids = list<string>();
	//Start by collecting comparisons with genealogically prior witnesses in collation order:
	list<const genealogical_comparison *> ancestors;
	for (const string & other_id : list_wit) {
		const auto & comp = genealogical_comparisons.at(other_id);
		if (comp.posterior.cardinality() > comp.prior.cardinality()) {
			ancestors.push_back(&comp);
		}
	}
	//Then sort this list by number of agreements:
	ancestors.sort([](const genealogical_comparison * c1, const genealogical_comparison * c2) {
		return c1->agreements.cardinality() > c2->agreements.cardinality();
	});
	//Now iterate through the sorted list,
	//copying only the IDs of the witnesses that are genealogically prior to this witness:
	for (const auto * comp : ancestors) {
		potential_ancestor_ids.push_back(comp->secondary_wit);
	}
	//Initialize the stemmatic ancestors list as empty:
	stemmatic_ancestor_ids = list<string>();
}

/**
 * Alternative constructor for a witness using an ID and a list of genealogical comparisons.
 * The list of genealogical comparisons should be ordered by secondary witness ID 
 * according to the order in which the witnesses are listed in the apparatus's list_wit member.
 */
witness::witness(const string & _id, const list<genealogical_comparison> & _genealogical_comparisons) {
	//Set its ID:
	id = _id;
	//Then populate the its map of genealogical_comparisons, keyed by witness ID:
	genealogical_comparisons = unordered_map<string, genealogical_comparison>();
	for (genealogical_comparison comp : _genealogical_comparisons) {
		string other_id = comp.secondary_wit;
		genealogical_comparisons[other_id] = comp;
	}
	//Next, populate this witness's list of potential ancestors:
	potential_ancestor_ids = list<string>();
	//Start by constructing a list of genealogical comparisons with all other witnesses:
	list<genealogical_comparison> comps = list<genealogical_comparison>(_genealogical_comparisons);
	//Then sort this list by number of agreements:
	comps.sort([](const genealogical_comparison & c1, const genealogical_comparison & c2) {
		return c1.agreements.cardinality() > c2.agreements.cardinality();
	});
	//Now iterate through the sorted list,
	//copying only the IDs of the witnesses that are genealogically prior to this witness:
	for (genealogical_comparison comp : comps) {
		string other_id = comp.secondary_wit;
		if (comp.posterior.cardinality() > comp.prior.cardinality()) {
			potential_ancestor_ids.push_back(other_id);
		}
	}
	//Initialize the stemmatic ancestors list as empty:
	stemmatic_ancestor_ids = list<string>();
}

/**
 * Default destructor.
 */
witness::~witness() {

}

/**
 * Returns the ID of this witness.
 */
string witness::get_id() const {
	return id;
}

/**
 * Returns this witness's map of genealogical comparisons, keyed by witness ID.
 */
unordered_map<string, genealogical_comparison> witness::get_genealogical_comparisons() const {
	return genealogical_comparisons;
}

/**
 * Returns a genealogical comparison between this witness and the witness with the given ID.
 */
genealogical_comparison witness::get_genealogical_comparison_for_witness(const string & other_id) const {
	return genealogical_comparisons.at(other_id);
}

/**
 * Returns a list of this witness's potential ancestors' IDs, sorted by pregenealogical coherence.
 */
list<string> witness::get_potential_ancestor_ids() const {
	return potential_ancestor_ids;
}

/**
 * Returns a list of all minimum-cost substemmata for this witness.
 * Optionally, an upper bound on substemma cost can be specified,
 * in which case all substemmata within that cost bound will be returned.
 * A boolean flag indicating whether a single solution is desired can also be specified,
 * in which case the cost bound will be ignored and an optimized version of the branch-and-bound procedure will be used.
 */
list<set_cover_solution> witness::get_substemmata(float ub, bool single_solution) const {
	return get_substemmata(ub, single_solution, set_cover_tie_policy::preserve_existing);
}

list<set_cover_solution> witness::get_substemmata(float ub, bool single_solution, set_cover_tie_policy tie_policy) const {
	list<set_cover_solution> substemmata = list<set_cover_solution>();
	//Populate a vector of set cover rows using genealogical comparisons with this witness's potential ancestors:
	vector<set_cover_row> rows = vector<set_cover_row>();
	for (string ancestor_id : potential_ancestor_ids) {
		genealogical_comparison comp = genealogical_comparisons.at(ancestor_id);
		set_cover_row row;
		row.id = ancestor_id;
		row.agreements = comp.agreements;
		row.explained = comp.explained;
		row.cost = comp.cost;
		rows.push_back(row);
	}
	//Sort this vector by increasing cost and decreasing number of agreements:
	stable_sort(begin(rows), end(rows), [](const set_cover_row & r1, const set_cover_row & r2) {
		return r1.cost < r2.cost ? true : (r1.cost > r2.cost ? false : (r1.agreements.cardinality() > r2.agreements.cardinality()));
	});
	//Initialize the bitmap of the target set to be covered:
	Roaring target = genealogical_comparisons.at(id).extant;
	//Then populate the rows of this table using the solver:
	set_cover_solver solver = (ub > 0 && !single_solution) ? set_cover_solver(rows, target, ub) : set_cover_solver(rows, target);
	solver.solve(substemmata, single_solution, tie_policy);
	return substemmata;
 }

/**
 * Populates this witness's substemma with the witness IDs in the given list.
 */
void witness::set_stemmatic_ancestor_ids(const list<string> & witnesses) {
	stemmatic_ancestor_ids = list<string>(witnesses);
	return;
}

/**
 * Returns this witness's list of stemmatic ancestor IDs.
 */
list<string> witness::get_stemmatic_ancestor_ids() const {
	return stemmatic_ancestor_ids;
}


