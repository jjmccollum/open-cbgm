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
 * Alternative constructor for a "partial witness" relative to a set of other witnesses.
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
 * Returns a list of this witness's potential ancestors' IDs, sorted by pregenealogical coherence.
 */
list<string> witness::get_potential_ancestor_ids() {
	return potential_ancestor_ids;
}

/**
 * Returns a set of IDs for all witnesses that are direct ancestors to this witness in textual flow diagrams.
 */
unordered_set<string> witness::get_textual_flow_ancestor_ids() {
	return textual_flow_ancestor_ids;
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
 * Given a list of witnesses, populates this witness's list of potential ancestor IDs,
 * sorting the other witnesses by their pregenealogical similarity with this witness
 * and filtering them based on their genealogical priority relative to this witness.
 */
void witness::set_potential_ancestor_ids(list<witness> wits) {
	potential_ancestor_ids = list<string>();
	//Sort the input list by pregenealogical similarity to this witness:
	wits.sort([this](witness & w1, witness & w2) {
		return pregenealogical_comp(w1, w2);
	});
	//Now iterate through the sorted list,
	//copying over only the IDs of the witnesses that are genealogically equal or prior to this witness
	//and are not the same as this witness:
	for (witness wit : wits) {
		string other_id = wit.get_id();
		if (get_explained_readings_for_witness(other_id).cardinality() >= wit.get_explained_readings_for_witness(id).cardinality() && other_id != id) {
			potential_ancestor_ids.push_back(other_id);
		}
	}
	return;
}

/**
 * Adds the given textual flow ancestor's witness ID to this witness's set of textual flow ancestors.
 */
void witness::add_textual_flow_ancestor_id(string ancestor_id) {
	textual_flow_ancestor_ids.insert(ancestor_id);
	return;
}

/**
 * Returns the smallest subset of this witness's set of textual flow ancestors that explains this witness's readings.
 * To find this subset, we use a brute-force algorithm for solving the minimum set cover problem.
 * Because of the combinatorial nature of this problem, its runtime will be O(k*n^k),
 * where n is the number of textual flow ancestors and k is the size of the minimum set cover.
 */
unordered_set<string> witness::get_global_stemma_ancestors() {
	unordered_set<string> global_stemma_ancestors = unordered_set<string>();
	//The set that needs to be covered is this witness's set of extant readings:
	Roaring target_set = explained_readings_by_witness[id];
	//Populate a vector of explained reading bitmaps:
	vector<string> ancestor_ids;
	vector<Roaring> explained_readings;
	for (string textual_flow_ancestor_id : textual_flow_ancestor_ids) {
		ancestor_ids.push_back(textual_flow_ancestor_id);
		explained_readings.push_back(explained_readings_by_witness[textual_flow_ancestor_id]);
	}
	//Before we begin, check if the union of all the bitmaps explains all of this witness's readings:
	Roaring union_all = Roaring();
	for (Roaring explained_reading : explained_readings) {
		union_all |= explained_reading;
	}
	if ((union_all & target_set).cardinality() < target_set.cardinality()) {
		//If it doesn't, then there is no set cover; report this to the user and return an empty set:
		cout << "Warning: Witness " << id << " has some readings that cannot be explained by all of its textual flow ancestors; consider updating the connectivity or local stemmata of one or more variation units." << endl;
		return global_stemma_ancestors;
	}
	//If the union explains all of this witness's readings, then proceed for each subset size, starting at 1:
	//TODO: This might be accelerated slightly using 64-bit integers and the instructions listed at https://graphics.stanford.edu/~seander/bithacks.html.
	bool found_set_cover = false;
	list<unordered_set<unsigned int>> solutions = list<unordered_set<unsigned int>>();
	for (unsigned int i = 1; i <= explained_readings.size(); i++) {
		vector<bool> vec(explained_readings.size(), false); //this boolean vector will encode the current combination
		fill(vec.begin(), vec.begin() + i, true); //set the first i entries to true as an initial combination
		do {
			//Initialize a set of indices describing the current subset of explained reading bitmaps:
			unordered_set<unsigned int> comb = unordered_set<unsigned int>();
			//Using the current permutation of the boolean vector, populate the combination indices set:
			for (unsigned int j = 0; j < vec.size(); j++) {
				if (vec[j]) {
					comb.insert(j);
				}
				if (comb.size() == i) {
					break;
				}
			}
			//Check if the bitmaps in this combination explain all of this witness's readings:
			Roaring explained_by_subset = Roaring();
			for (unsigned int j : comb) {
				explained_by_subset |= explained_readings[j];
			}
			if ((explained_by_subset & target_set).cardinality() == target_set.cardinality()) {
				//If they do, then we have a set cover; set the flag indicating that we have found a set cover of this size
				//and add the current subset indices to the solutions list:
				found_set_cover = true;
				solutions.push_back(comb);
			}
		} while(next_permutation(vec.begin(), vec.end()));
		//If we've found any set covers of this size, then there's no need to check for larger ones:
		if (found_set_cover) {
			break;
		}
	}
	//Choose the set cover with the highest total number of agreements with this witness:
	int best_total = 0;
	unordered_set<string> best_cover;
	for (unordered_set<unsigned int> solution : solutions) {
		int total_agreements = 0;
		unordered_set<string> ancestors_in_solutions = unordered_set<string>();
		for (unsigned int i : solution) {
			string ancestor_id = ancestor_ids[i];
			total_agreements += agreements_by_witness[ancestor_id].cardinality();
			ancestors_in_solutions.insert(ancestor_id);
		}
		if (total_agreements > best_total) {
			best_total = total_agreements;
			best_cover = ancestors_in_solutions;
		}
	}
	global_stemma_ancestors = best_cover;
	return global_stemma_ancestors;
}
