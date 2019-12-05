/*
 * optimize_substemmata.cpp
 *
 *  Created on: Dec 2, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <list>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <getopt.h>

#include "pugixml.h"
#include "roaring.hh"
#include "witness.h"
#include "apparatus.h"
#include "variation_unit.h"

using namespace std;

/**
 * Data structure representing a set cover row, including the ID of the witness it represents and its cost.
 */
struct set_cover_row {
	string id;
	Roaring bits;
	int cost;
};

/**
 * Prints the short usage message.
 */
void usage() {
	printf("usage: optimize_substemmata [-h] [-t threshold] [--split] [--orth] [--def] input_xml witness\n\n");
	return;
}

/**
 * Prints the help message.
 */
void help() {
	usage();
	printf("Get a table of best-found substemmata for the witness with the given ID.\n\n");
	printf("optional arguments:\n");
	printf("\t-h, --help: print usage manual\n");
	printf("\t-t, --threshold: minimum extant readings threshold\n\n");
	printf("\t--split: treat split attestations as distinct readings");
	printf("\t--orth: treat orthographic subvariants as distinct readings");
	printf("\t--def: treat defective forms as distinct readings");
	printf("positional arguments:\n");
	printf("\tinput_xml: collation file in TEI XML format\n");
	printf("\twitness: ID of the witness whose substemmata are desired, as found in its <witness> element in the XML file\n");
	return;
}

/**
 * Given a target set to be covered, a vector of row data structures,
 * a set of current accepted row indices, and a set of current rejected row indices,
 * returns the index of the next row to branch on.
 */
unsigned int get_next_branch(Roaring target, vector<set_cover_row> rows, unordered_set<unsigned int> remaining) {
	//The next row to branch on is the unprocessed row with the lowest cost-to-coverage proportion:
	float best_density = float(target.cardinality());
	unsigned int best_row_ind;
	for (unsigned int row_ind : remaining) {

	}
}

/**
 * Returns the cost of a solution (assumed to be feasible).
 */
unsigned int get_cost(vector<set_cover_row> rows, unordered_set<unsigned int> solution) {
	unsigned int cost = 0;
	for (unsigned int row_ind : solution) {
		set_cover_row row = rows[row_ind];
		cost += row.cost;
	}
	return cost;
}

/**
 * Given a vector of row data structures and a target bitmap to cover,
 * returns the set of row indices corresponding to the solution found by the basic greedy heuristic.
 */
unordered_set<unsigned int> get_greedy_solution(vector<set_cover_row> rows, Roaring target) {
	Roaring initial_greedy_solution_bitmap = Roaring();
	Roaring temp_target = Roaring(target);
	//Until the target is completely covered, choose the row with the lowest cost-to-coverage proportion:
	while (temp_target.cardinality() > 0) {
		float best_density = float(target.cardinality());
		unsigned int best_row_ind;
		unsigned int row_ind = 0;
		for (set_cover_row row : rows) {
			int cost = row.cost;
			int coverage = (temp_target & row.bits).cardinality();
			//Skip if there is no coverage:
			if (coverage == 0) {
				row_ind++;
				continue;
			}
			float density = float(cost) / float(coverage);
			if (density < best_density && !initial_greedy_solution_bitmap.contains(row_ind)) {
				best_density = density;
				best_row_ind = row_ind;
			}
			row_ind++;
		}
		//Add the best-found row to the solution, and remove its overlap with the target set from the target set:
		initial_greedy_solution_bitmap.add(best_row_ind);
		temp_target ^= temp_target & row.bits;
	}
	//Now loop backwards through the set of added row indices to remove the highest-cost redundant columns:
	unordered_set<unsigned int> reduced_greedy_solution = unordered_set<unsigned int>();
	for (Roaring::const_iterator row_ind = initial_greedy_solution_bitmap.begin(); row_ind != initial_greedy_solution_bitmap.end(); row_ind++) {
		reduced_greedy_solution.insert(row_ind);
	}
	while (initial_greedy_solution.cardinality() > 0) {
		//Get the last-indexed (i.e., highest-cost) row:
		unsigned int row_ind;
		initial_greedy_solution_bitmap.select(initial_greedy_solution_bitmap.cardinality() - 1, & row_ind);
		//Check if it is redundant (i.e., if the other rows still in the reduced solution set can cover the original target set):
		reduced_greedy_solution.erase(row_ind);
		Roaring row_union = Roaring();
		for (unsigned int other_row_ind : reduced_greedy_solution) {
			row_union |= rows[other_row_ind].bits;
		}
		if (!target.isSubset(row_union)) {
			reduced_greedy_solution.insert(row_ind);
		}
		initial_greedy_solution_bitmap.remove(row_ind);
	}
	return reduced_greedy_solution;
}

/**
 * Prints a substemma for the primary witness, along with its total cost.
 */
void print_substemma(vector<set_cover_row> rows, set<unsigned int> solution) {
	for (unsigned int row_ind : solution) {
		set_cover_row row = rows[row_ind];
		if (row_ind != solution.begin()) {
			cout << ", ";
		}
		cout << row.id;
	}
	cout << "\t" << get_cost(rows, solution) << endl;
	return;
}

/**
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Parse the command-line options:
	int split = 0;
	int orth = 0;
	int def = 0;
	unsigned int threshold = 0;
	const char* const short_opts = "ht:";
	const option long_opts[] = {
		{"split", no_argument, & split, 1},
		{"orth", no_argument, & orth, 1},
		{"def", no_argument, & def, 1},
		{"threshold", required_argument, nullptr, 't'},
		{"help", no_argument, nullptr, 'h'},
		{nullptr, no_argument, nullptr, 0}
	};
	int opt;
	while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (opt) {
			case 'h':
				help();
				return 0;
			case 't':
				threshold = atoi(optarg);
				break;
			case 0:
				//This will happen if a long option is being parsed; just move on:
				break;
			default:
				printf("Error: invalid argument.\n");
				usage();
				exit(1);
		}
	}
	//Parse the positional arguments:
	int index = optind;
	if (argc <= index + 1) {
		printf("Error: 2 positional arguments (input_xml and witness) are required.\n");
		exit(1);
	}
	//The first positional argument is the XML file:
	char * input_xml = argv[index];
	index++;
	//The next argument is the primary witness ID:
	string primary_wit_id = string(argv[index]);
	cout << "Calculating genealogical relationships between witness " << primary_wit_id << " and all other witnesses..." << endl;
	//Using the input flags, populate a set of reading types to be treated as distinct:
	unordered_set<string> distinct_reading_types = unordered_set<string>();
	if (split) {
		//Treat split readings as distinct:
		distinct_reading_types.insert("split");
	}
	if (orth) {
		//Treat orthographic variants as distinct:
		distinct_reading_types.insert("orthographic");
	}
	if (def) {
		//Treat defective variants as distinct:
		distinct_reading_types.insert("defective");
	}
	//Attempt to parse the input XML file as an apparatus:
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(input_xml);
	if (!result) {
		printf("Error: An error occurred while loading XML file %s: %s\n", input_xml, result.description());
		exit(1);
	}
	pugi::xml_node tei_node = doc.child("TEI");
	if (!tei_node) {
		printf("Error: The XML file %s does not have a <TEI> element as its root element.\n", input_xml);
		exit(1);
	}
	apparatus app = apparatus(tei_node, distinct_reading_types);
	//Ensure that the primary witness is included in the apparatus's <listWit> element:
	unordered_set<string> list_wit = app.get_list_wit();
	if (list_wit.find(primary_wit_id) == list_wit.end()) {
		printf("Error: The XML file's <listWit> element has no child <witness> element with ID %s.\n", primary_wit_id.c_str());
		exit(1);
	}
	//Populate a set of IDs for all other witnesses that match the input parameters
	//and a map of the witnesses themselves, keyed by ID:
	unordered_set<string> secondary_wit_ids = unordered_set<string>();
	unordered_map<string, witness> secondary_witnesses_by_id = unordered_map<string, witness>();
	for (string secondary_wit_id : list_wit) {
		//Skip the primary witness:
		if (secondary_wit_id == primary_wit_id) {
			continue;
		}
		//Initialize a secondary witness relative to the primary witness:
		unordered_set<string> wit_ids = unordered_set<string>({primary_wit_id, secondary_wit_id});
		witness secondary_wit = witness(secondary_wit_id, wit_ids, app);
		//If it is too lacunose, then ignore it:
		if (secondary_wit.get_explained_readings_for_witness(secondary_wit_id).cardinality() < threshold) {
			continue;
		}
		//Otherwise, add it:
		secondary_wit_ids.insert(secondary_wit_id);
		secondary_witnesses_by_id[secondary_wit_id] = secondary_wit;
	}
	//Initialize the primary witness relative to all of the secondary witnesses in this map:
	secondary_wit_ids.insert(primary_wit_id);
	witness primary_wit = witness(primary_wit_id, secondary_wit_ids, app);
	//Now populate the primary witness's list of potential ancestors:
	primary_wit.set_potential_ancestor_ids(secondary_witnesses_by_id);
	//Using this list, populate a vector of set cover rows consisting of the bitmaps
	//representing the readings in the primary witness explained by the secondary witness,
	//along with their costs:
	vector<set_cover_row> rows = vector<set_cover_row>();
	for (string secondary_wit_id : primary_wit.get_potential_ancestor_ids()) {
		set_cover_row row;
		row.id = secondary_wit_id;
		row.bits = primary_wit.get_explained_readings_for_witness(secondary_wit_id);
		row.cost = (primary_wit.get_explained_readings_for_witness(primary_wit_id) ^ primary_wit.get_agreements_for_witness(secondary_wit_id)).cardinality();
		rows.push_back(row);
	}
	//Likewise, populate a vector of bitmaps representing the columns of the covering matrix:
	vector<Roaring> cols = vector<Roaring>(app.get_variation_units().size());
	unsigned int row_ind = 0;
	for (set_cover_row row : rows) {
		for (Roaring::const_iterator col_ind = row.bits.begin(); col_ind != row.bits.end(); col_ind++) {
			cols[col_ind].add(row_ind);
		}
		row_ind++;
	}
	//Initialize the bitmap of the target set to be covered:
	Roaring target = primary_wit.get_explained_readings_for_witness(primary_wit_id);
	//Initialize sets to contain candidate rows for a given covering:
	unordered_set<unsigned int> accepted = unordered_set<unsigned int>();
	unordered_set<unsigned int> rejected = unordered_set<unsigned int>();
	//If any column in the target set is not covered by any row, then there is no solution:
	unordered_set<unsigned int> uncovered = unordered_set<unsigned int>();
	for (Roaring::const_iterator col_ind = target.begin(); col_ind != target.end(); col_ind++) {
		if (cols[col_ind].cardinality() == 0) {
			uncovered.insert(col_ind);
		}
	}
	if (uncovered.size() > 0) {
		cout << "The witness with ID" << primary_wit_id << "cannot be explained by any of its potential ancestors at the following variation units: ";
		unsigned int vu_ind = 0;
		for (variation_unit vu : app.get_variation_units()) {
			if (uncovered.find(vu_ind) != uncovered.end()) {
				cout << vu.get_label() << " ";
			}
			vu_ind++;
		}
		cout << endl;
		exit(0);
	}
	//Otherwise, if any column in the target set is uniquely covered by a row, then that row must be included in the solution,
	//and the columns it covers can be removed from the target set;
	//we can set aside any such columns ahead of time and add them back in later:
	unordered_set<unsigned int> unique_row_inds = unordered_set<unsigned int>();
	for (Roaring::const_iterator col_ind = target.begin(); col_ind != target.end(); col_ind++) {
		if (cols[col_ind].cardinality() == 1) {
			unsigned int row_ind = cols[col_ind].select(0);
			unique_row_inds.insert(row_ind);
		}
	}
	for (unsigned int row_ind : unique_row_inds) {
		accepted.insert(row_ind);
		set_cover_row row = rows[row_ind];
		target ^= target & row.bits;
	}
	//If the remaining set is empty, then the rows with unique column coverage
	//represent the witnesses in the only valid substemma for the primary witness:
	if (target.cardinality() == 0) {
		print_substemma(rows, accepted);
	}
	//Otherwise, proceed with branch and bound on the remaining target set and candidate rows;
	//using the greedy solution to get a good initial upper bound on the cost:
	unordered_set<unsigned int> greedy_solution = get_greedy_solution(rows, target);
	int ub = get_cost(rows, greedy_solution);
	return 0;
}
