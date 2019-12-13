/*
 * Witness.cpp
 *
 *  Created on: Oct 24, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

#include "roaring.hh"
#include "witness.h"
#include "set_cover_solver.h"
#include "apparatus.h"
#include "variation_unit.h"
#include "local_stemma.h"

using namespace std;

/**
 * Default constructor.
 */
witness::witness() {

}

/**
 * Constructs a witness using its ID and a textual apparatus.
 */
witness::witness(string witness_id, apparatus app) {
	//Set its ID:
	id = witness_id;
	//Now populate the its maps of agreements and explained readings, keyed by other witnesses:
	agreements_by_witness = unordered_map<string, Roaring>();
	explained_readings_by_witness = unordered_map<string, Roaring>();
	unordered_set<string> list_wit = app.get_list_wit();
	for (string other_id : list_wit) {
		Roaring equal_readings = Roaring(); //readings in the other witness equal to this witness's readings
		Roaring equal_or_prior_readings = Roaring(); //readings in the other witness equal or prior to this witness's readings
		int variation_unit_index = 0;
		for (variation_unit vu : app.get_variation_units()) {
			//Get the indices of all readings supported by each of these witnesses at this variation unit
			//(the Ausgangstext A may support more than one reading):
			unordered_map<string, list<string>> reading_support = vu.get_reading_support();
			list<string> readings_for_this = (reading_support.find(witness_id) != reading_support.end()) ? reading_support[witness_id] : list<string>();
			list<string> readings_for_other = (reading_support.find(other_id) != reading_support.end()) ? reading_support[other_id] : list<string>();
			//If either witness's list is empty, then it is lacunose here,
			//and there is no relationship (including equality, as two lacunae should not be treated as equal):
			if (readings_for_this.size() == 0 || readings_for_other.size() == 0) {
				variation_unit_index++;
				continue;
			}
			//Otherwise, loop through all pairs of readings:
			local_stemma ls = vu.get_local_stemma();
			bool is_equal = false;
			bool is_equal_or_prior = false;
			for (string reading_for_this : readings_for_this) {
				for (string reading_for_other : readings_for_other) {
					is_equal |= (reading_for_this == reading_for_other);
					is_equal_or_prior |= ls.is_equal_or_prior(reading_for_other, reading_for_this);
				}
			}
			if (is_equal) {
				equal_readings.add(variation_unit_index);
				equal_or_prior_readings.add(variation_unit_index);
			}
			else if (is_equal_or_prior) {
				equal_or_prior_readings.add(variation_unit_index);
			}
			variation_unit_index++;
		}
		agreements_by_witness[other_id] = equal_readings;
		explained_readings_by_witness[other_id] = equal_or_prior_readings;
	}
}

/**
 * Alternative constructor for a witness relative to a set of other witnesses.
 * This constructor only populates the witness's agreements and explained readings bitmaps relative to itself and the specified witnesses.
 */
witness::witness(string witness_id, unordered_set<string> list_wit, apparatus app) {
	//Set its ID:
	id = witness_id;
	//Now populate the its maps of agreements and explained readings, keyed by other witnesses:
	agreements_by_witness = unordered_map<string, Roaring>();
	explained_readings_by_witness = unordered_map<string, Roaring>();
	for (string other_id : list_wit) {
		Roaring equal_readings = Roaring(); //readings in the other witness equal to this witness's readings
		Roaring equal_or_prior_readings = Roaring(); //readings in the other witness equal or prior to this witness's readings
		int variation_unit_index = 0;
		for (variation_unit vu : app.get_variation_units()) {
			//Get the indices of all readings supported by each of these witnesses at this variation unit
			//(the Ausgangstext A may support more than one reading):
			unordered_map<string, list<string>> reading_support = vu.get_reading_support();
			list<string> readings_for_this = (reading_support.find(witness_id) != reading_support.end()) ? reading_support[witness_id] : list<string>();
			list<string> readings_for_other = (reading_support.find(other_id) != reading_support.end()) ? reading_support[other_id] : list<string>();
			//If either witness's list is empty, then it is lacunose here,
			//and there is no relationship (including equality, as two lacunae should not be treated as equal):
			if (readings_for_this.size() == 0 || readings_for_other.size() == 0) {
				variation_unit_index++;
				continue;
			}
			//Otherwise, loop through all pairs of readings:
			local_stemma ls = vu.get_local_stemma();
			bool is_equal = false;
			bool is_equal_or_prior = false;
			for (string reading_for_this : readings_for_this) {
				for (string reading_for_other : readings_for_other) {
					is_equal |= (reading_for_this == reading_for_other);
					is_equal_or_prior |= ls.is_equal_or_prior(reading_for_other, reading_for_this);
				}
			}
			if (is_equal) {
				equal_readings.add(variation_unit_index);
				equal_or_prior_readings.add(variation_unit_index);
			}
			else if (is_equal_or_prior) {
				equal_or_prior_readings.add(variation_unit_index);
			}
			variation_unit_index++;
		}
		agreements_by_witness[other_id] = equal_readings;
		explained_readings_by_witness[other_id] = equal_or_prior_readings;
	}
}

/**
 * Default destructor.
 */
witness::~witness() {

}

/**
 * Returns the ID of this witness.
 */
string witness::get_id() {
	return id;
}

/**
 * Returns the bitset representing the agreements of this witness with other witnesses,
 * keyed by the other witness's ID.
 */
unordered_map<string, Roaring> witness::get_agreements_by_witness() {
	return agreements_by_witness;
}

/**
 * Returns a map of bitsets representing the readings of this witness explained by other witnesses,
 * keyed by the other witness's ID.
 */
unordered_map<string, Roaring> witness::get_explained_readings_by_witness() {
	return explained_readings_by_witness;
}

/**
 * Returns a bitset indicating the units at which this witness agrees with the witness with the given ID.
 */
Roaring witness::get_agreements_for_witness(string other_id) {
	return agreements_by_witness[other_id];
}

/**
 * Returns a bitset indicating which of this witness's readings are explained by the witness with the given ID.
 * If this witness's ID is the input, then the bitset indicates the units at which this witness is extant.
 */
Roaring witness::get_explained_readings_for_witness(string other_id) {
	return explained_readings_by_witness[other_id];
}

/**
 * Computes the pregenealogical similarity of the two given witnesses to this witness
 * and returns a boolean value indicating whether the similarity to the first is greater than the similarity to the second.
 */
bool witness::pregenealogical_comp(witness & w1, witness & w2) {
	Roaring agreements_with_w1 = agreements_by_witness[w1.get_id()];
	Roaring agreements_with_w2 = agreements_by_witness[w2.get_id()];
	Roaring extant = explained_readings_by_witness[id];
	float pregenealogical_similarity_to_w1 = float(agreements_with_w1.cardinality()) / float(extant.cardinality());
	float pregenealogical_similarity_to_w2 = float(agreements_with_w2.cardinality()) / float(extant.cardinality());
	return pregenealogical_similarity_to_w1 > pregenealogical_similarity_to_w2;
}

/**
 * Returns a list of this witness's potential ancestors' IDs, sorted by pregenealogical coherence.
 */
list<string> witness::get_potential_ancestor_ids() {
	return potential_ancestor_ids;
}

/**
 * Given a map of witnesses keyed by ID, populates this witness's list of potential ancestor IDs,
 * sorting the other witnesses by their pregenealogical similarity with this witness
 * and filtering them based on their genealogical priority relative to this witness.
 */
void witness::set_potential_ancestor_ids(unordered_map<string, witness> witnesses_by_id) {
	potential_ancestor_ids = list<string>();
	list<witness> wits = list<witness>();
	for (pair<string, witness> kv : witnesses_by_id) {
		wits.push_back(kv.second);
	}
	//Sort the input list by pregenealogical similarity to this witness:
	wits.sort([this](witness & w1, witness & w2) {
		return pregenealogical_comp(w1, w2);
	});
	//Now iterate through the sorted list,
	//copying over only the IDs of the witnesses that are genealogically prior to this witness:
	for (witness wit : wits) {
		string other_id = wit.get_id();
		if (get_explained_readings_for_witness(other_id).cardinality() > wit.get_explained_readings_for_witness(id).cardinality()) {
			potential_ancestor_ids.push_back(other_id);
		}
	}
	return;
}

/**
 * Returns this witness's list of global stemma ancestor IDs.
 */
list<string> witness::get_global_stemma_ancestors() {
	return global_stemma_ancestor_ids;
}

/**
 * Identifies the witnesses found in the optimal substemma for this witness.
 * The results are stored in this witness's global_stemma_ancestor_ids list.
 */
void witness::set_global_stemma_ancestors() {
	global_stemma_ancestor_ids = list<string>();
	//Populate a vector of set cover rows using the explained readings bitmaps for this witness's potential ancestors:
	vector<set_cover_row> rows = vector<set_cover_row>();
	for (string secondary_wit_id : potential_ancestor_ids) {
		set_cover_row row;
		row.id = secondary_wit_id;
		row.bits = explained_readings_by_witness[secondary_wit_id];
		row.cost = (explained_readings_by_witness[id] ^ agreements_by_witness[secondary_wit_id]).cardinality();
		rows.push_back(row);
	}
	//Initialize the bitmap of the target set to be covered:
	Roaring target = explained_readings_by_witness[id];
	//Initialize the list of solutions to be populated:
	list<set_cover_solution> solutions;
	//Then populate it using the solver:
	set_cover_solver solver = set_cover_solver(rows, target);
	solver.solve(solutions);
	//If it is not empty, then add the IDs corresponding to the optimal solution:
	if (!solutions.empty()) {
		set_cover_solution solution = solutions.front();
		for (set_cover_row row : solution.rows) {
			global_stemma_ancestor_ids.push_back(row.id);
		}
	}
	return;
}
