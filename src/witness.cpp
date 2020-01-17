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
#include <limits>

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
witness::witness(const string & _id, const apparatus & app) {
	//Set its ID:
	id = _id;
	//Now populate the its map of genealogical_comparisons, keyed by witness ID:
	genealogical_comparisons = unordered_map<string, genealogical_comparison>();
	list<string> list_wit = app.get_list_wit();
	for (string other_id : list_wit) {
		//Initialize a genealogical_comparison data structure for this witness:
		genealogical_comparison comp;
		comp.agreements = Roaring(); //readings in the other witness equal to this witness's readings
		comp.explained = Roaring(); //readings in the other witness equal or prior to this witness's readings
		comp.cost = 0; //genealogical cost of the other witness relative to this witness
		int vu_ind = 0;
		for (variation_unit vu : app.get_variation_units()) {
			//Try to get the reading of each witness at this variation unit:
			unordered_map<string, string> reading_support = vu.get_reading_support();
			//If either witness is lacunose, then there is no relationship
			//(including equality, as two lacunae should not be treated as equal):
			if (reading_support.find(id) == reading_support.end() || reading_support.find(other_id) == reading_support.end()) {
				vu_ind++;
				continue;
			}
			//Otherwise, check for a path from the other witness's reading to this one in the local stemma:
			string reading_for_this = reading_support.at(id);
			string reading_for_other = reading_support.at(other_id);
			local_stemma ls = vu.get_local_stemma();
			float path_length = numeric_limits<float>::infinity();
			if (ls.path_exists(reading_for_other, reading_for_this)) {
				path_length = ls.get_shortest_path_length(reading_for_other, reading_for_this);
			}
			if (path_length < numeric_limits<float>::infinity()) {
				comp.explained.add(vu_ind);
				if (path_length == 0) {
					comp.agreements.add(vu_ind);
				}
				comp.cost += path_length;
			}
			vu_ind++;
		}
		//Add the completed genealogical_comparison to this witness's map:
		genealogical_comparisons[other_id] = comp;
	}
}

/**
 * Alternative constructor for a witness relative to a list of other witnesses.
 * This constructor only populates the witness's agreements and explained readings bitmaps relative to itself and the specified witnesses.
 */
witness::witness(const string & _id, const list<string> & list_wit, const apparatus & app) {
	//Set its ID:
	id = _id;
	//Now populate the its map of genealogical_comparisons, keyed by witness ID:
	genealogical_comparisons = unordered_map<string, genealogical_comparison>();
	for (string other_id : list_wit) {
		//Initialize a genealogical_comparison data structure for this witness:
		genealogical_comparison comp;
		comp.agreements = Roaring(); //readings in the other witness equal to this witness's readings
		comp.explained = Roaring(); //readings in the other witness equal or prior to this witness's readings
		comp.cost = 0; //genealogical cost of the other witness relative to this witness
		int vu_ind = 0;
		for (variation_unit vu : app.get_variation_units()) {
			//Try to get the reading of each witness at this variation unit:
			unordered_map<string, string> reading_support = vu.get_reading_support();
			//If either witness is lacunose, then there is no relationship
			//(including equality, as two lacunae should not be treated as equal):
			if (reading_support.find(id) == reading_support.end() || reading_support.find(other_id) == reading_support.end()) {
				vu_ind++;
				continue;
			}
			//Otherwise, check for a path from the other witness's reading to this one in the local stemma:
			string reading_for_this = reading_support.at(id);
			string reading_for_other = reading_support.at(other_id);
			local_stemma ls = vu.get_local_stemma();
			float path_length = numeric_limits<float>::infinity();
			if (ls.path_exists(reading_for_other, reading_for_this)) {
				path_length = ls.get_shortest_path_length(reading_for_other, reading_for_this);
			}
			if (path_length < numeric_limits<float>::infinity()) {
				comp.explained.add(vu_ind);
				if (path_length == 0) {
					comp.agreements.add(vu_ind);
				}
				comp.cost += path_length;
			}
			vu_ind++;
		}
		//Add the completed genealogical_comparison to this witness's map:
		genealogical_comparisons[other_id] = comp;
	}
}

/**
 * Alternative constructor for a witness using an ID and a map of genealogical comparisons populated using the genealogical cache.
 */
witness::witness(const string & _id, const unordered_map<string, genealogical_comparison> & _genealogical_comparisons) {
	//Set its ID:
	id = _id;
	//Then populate the its map of genealogical_comparisons, keyed by witness ID:
	genealogical_comparisons = _genealogical_comparisons;
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
 * Gets the genealogical_comparisons for the two given witnesses relative to this witness
 * and returns a boolean value indicating whether the number of agreements with the first is greater than the number of agreements with the second.
 */
bool witness::potential_ancestor_comp(const witness & w1, const witness & w2) const {
	genealogical_comparison w1_comp = genealogical_comparisons.at(w1.get_id());
	genealogical_comparison w2_comp = genealogical_comparisons.at(w2.get_id());
	return w1_comp.agreements.cardinality() > w2_comp.agreements.cardinality();
}

/**
 * Returns a list of this witness's potential ancestors' IDs, sorted by pregenealogical coherence.
 */
list<string> witness::get_potential_ancestor_ids() const {
	return potential_ancestor_ids;
}

/**
 * Given a list of witnesses, populates this witness's list of potential ancestor IDs,
 * sorting the other witnesses by genealogical cost relative to this witness
 * and filtering out any witnesses not genealogically prior to this witness.
 */
void witness::set_potential_ancestor_ids(const list<witness> & witnesses) {
	potential_ancestor_ids = list<string>();
	list<witness> wits = list<witness>(witnesses);
	//Sort the input list by number of agreements with this witness:
	wits.sort([this](const witness & w1, const witness & w2) {
		return potential_ancestor_comp(w1, w2);
	});
	//Now iterate through the sorted list,
	//copying over only the IDs of the witnesses that are genealogically prior to this witness:
	for (witness wit : wits) {
		string wit_id = wit.get_id();
		genealogical_comparison comp = genealogical_comparisons.at(wit_id);
		genealogical_comparison other_comp = wit.get_genealogical_comparison_for_witness(id);
		if (comp.explained.cardinality() > other_comp.explained.cardinality()) {
			potential_ancestor_ids.push_back(wit_id);
		}
	}
	return;
}

/**
 * Returns this witness's list of global stemma ancestor IDs.
 */
list<string> witness::get_global_stemma_ancestor_ids() const {
	return global_stemma_ancestor_ids;
}

/**
 * Identifies the witnesses found in the optimal substemma for this witness.
 * The results are stored in this witness's global_stemma_ancestor_ids list.
 */
void witness::set_global_stemma_ancestor_ids() {
	global_stemma_ancestor_ids = list<string>();
	//Populate a vector of set cover rows using genealogical_comparisons for this witness's potential ancestors:
	vector<set_cover_row> rows = vector<set_cover_row>();
	for (string wit_id : potential_ancestor_ids) {
		genealogical_comparison comp = genealogical_comparisons.at(wit_id);
		set_cover_row row;
		row.id = wit_id;
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
	Roaring target = genealogical_comparisons.at(id).explained;
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
