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
#include <stack>
#include <getopt.h>

#include "pugixml.h"
#include "roaring.hh"
#include "witness.h"
#include "apparatus.h"
#include "variation_unit.h"

using namespace std;

/**
 * Enumeration of states for an accept-reject branch and bound node.
 */
enum node_state {ACCEPT, REJECT, DONE};

/**
 * Data structure representing a branch and bound node, including its processing state.
 */
struct branch_and_bound_node {
	unsigned int candidate_row;
	node_state state;
};

/**
 * Data structure representing a set cover row, including the ID of the witness it represents and its cost.
 */
struct set_cover_row {
	string id;
	Roaring bits;
	int cost;
};

/**
 * Data structure representing a set cover solution.
 */
struct set_cover_solution {
	list<set_cover_row> rows;
	int cost;
};


/**
 * Prints the short usage message.
 */
void usage() {
	printf("usage: optimize_substemmata [-h] [-t threshold] [-b bound] [--split] [--orth] [--def] input_xml witness\n\n");
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
	printf("\t-t, --threshold: minimum extant readings threshold\n");
	printf("\t-b, --bound: fixed upper bound on substemmata cost; if specified, list all substemmata with costs within this bound\n\n");
	printf("\t--split: treat split attestations as distinct readings");
	printf("\t--orth: treat orthographic subvariants as distinct readings");
	printf("\t--def: treat defective forms as distinct readings");
	printf("positional arguments:\n");
	printf("\tinput_xml: collation file in TEI XML format\n");
	printf("\twitness: ID of the witness whose substemmata are desired, as found in its <witness> element in the XML file\n");
	return;
}

/**
 * Given a list of set cover solutions (assumed to be sorted from lowest cost to highest),
 * prints the corresponding best-found substemmata for the primary witness, along with their costs.
 */
void print_substemmata(list<set_cover_solution> solutions) {
	//Print the header row first:
	cout << std::left << std::setw(64) << "SUBSTEMMA" << std::right << std::setw(8) << "COST" << "\n\n";
	for (set_cover_solution solution : solutions) {
		string solution_str = "";
		for (set_cover_row row : solution.rows) {
			if (row.id != solution.rows.front().id) {
				solution_str += ", ";
			}
			solution_str += row.id;
		}
		cout << std::left << std::setw(64) << solution_str << std::right << std::setw(8) << solution.cost;
		cout << "\n";
	}
	cout << endl;
	return;
}

/**
 * Given a vector of row data structures,
 * bitmaps representing the target set of columns to cover, rows contained in the current solution, and rows that have not yet been considered,
 * and a stack of candidate solutions to be evaluated,
 * identifies the next row to branch on and adds candidate solutions for it to the stack.
 */
void branch(vector<set_cover_row> rows, Roaring target, Roaring accepted, Roaring remaining, stack<branch_and_bound_node> & nodes) {
	//Update the target set using the current set of accepted rows:
	Roaring uncovered = Roaring(target);
	for (Roaring::const_iterator row_ind = accepted.begin(); row_ind != accepted.end(); row_ind++) {
		set_cover_row row = rows[* row_ind];
		uncovered ^= uncovered & row.bits;
	}
	//Find the first remaining row:
	unsigned int best_row_ind;
	remaining.select(0, & best_row_ind);
	//Now that the next row index has been identified, add a node for it:
	branch_and_bound_node next_node;
	next_node.candidate_row = best_row_ind;
	next_node.state = node_state::ACCEPT;
	nodes.push(next_node);
	return;
}

/**
 * Given a vector of row data structures and a bitmap representing rows that are currently included in a candidate solution,
 * returns a lower bound on the cost of any solution that contains this candidate solution as a subset.
 */
int bound(vector<set_cover_row> rows, Roaring accepted) {
	int bound = 0;
	for (Roaring::const_iterator row_ind = accepted.begin(); row_ind != accepted.end(); row_ind++) {
		set_cover_row row = rows[* row_ind];
		bound += row.cost;
	}
	return bound;
}

/**
 * Given a vector of row data structures
 * and bitmaps representing the target set of columns to cover and the rows contained in the current solution,
 * returns a boolean value indicating if the current set of accepted rows constitutes a feasible set cover solution.
 */
bool is_feasible(vector<set_cover_row> rows, Roaring target, Roaring accepted) {
	//Check if the target set is covered by the accepted rows:
	Roaring row_union = Roaring();
	for (Roaring::const_iterator row_ind = accepted.begin(); row_ind != accepted.end(); row_ind++) {
		set_cover_row row = rows[* row_ind];
		row_union |= row.bits;
		if (target.isSubset(row_union)) {
			return true;
		}
	}
	return false;
}

/**
 * Given a vector of row data structures
 * and bitmaps representing the target set of columns to cover, the rows contained in the current solution, and the rows that have not been processed yet,
 * returns a boolean value indicating if any solution containing the current one is feasible.
 */
bool is_any_branch_feasible(vector<set_cover_row> rows, Roaring target, Roaring accepted, Roaring remaining) {
	//Check if the target set is covered by the accepted and remaining rows:
	Roaring rows_in_branch = accepted | remaining;
	return is_feasible(rows, target, rows_in_branch);
}

/**
 * Given a vector of row data structures and a target bitmap to cover,
 * returns a trivial solution consisting of the lowest-cost row that covers the target columns.
 * If the Ausgangstext explains all other readings
 * (i.e., if all local stemmata are connected, which is necessary for the global stemma to be connected),
 * then at least one such solution is guaranteed to exist.
 */
set_cover_solution get_trivial_solution(vector<set_cover_row> rows, Roaring target) {
	set_cover_solution trivial_solution;
	trivial_solution.rows = list<set_cover_row>();
	trivial_solution.cost = target.cardinality();
	for (set_cover_row row : rows) {
		if (target.isSubset(row.bits) && row.cost < trivial_solution.cost) {
			trivial_solution.rows = list<set_cover_row>({row});
			trivial_solution.cost = row.cost;
		}
	}
	return trivial_solution;
}

/**
 * Given a vector of row data structures and a target bitmap to cover,
 * returns the solution found by the basic greedy heuristic.
 */
set_cover_solution get_greedy_solution(vector<set_cover_row> rows, Roaring target) {
	Roaring initial_greedy_solution_bitmap = Roaring();
	Roaring uncovered = Roaring(target);
	//Until the target is completely covered, choose the row with the lowest cost-to-coverage proportion:
	while (uncovered.cardinality() > 0) {
		float best_density = float(target.cardinality());
		unsigned int best_row_ind = 0;
		unsigned int row_ind = 0;
		for (set_cover_row row : rows) {
			int cost = row.cost;
			int coverage = (uncovered & row.bits).cardinality();
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
		set_cover_row best_row = rows[best_row_ind];
		uncovered ^= uncovered & best_row.bits;
	}
	//Now loop backwards through the set of added row indices to remove the highest-cost redundant columns:
	set_cover_solution reduced_greedy_solution;
	unordered_set<unsigned int> reduced_greedy_solution_candidates = unordered_set<unsigned int>();
	for (Roaring::const_iterator row_ind = initial_greedy_solution_bitmap.begin(); row_ind != initial_greedy_solution_bitmap.end(); row_ind++) {
		reduced_greedy_solution_candidates.insert(* row_ind);
	}
	reduced_greedy_solution.cost = 0;
	while (initial_greedy_solution_bitmap.cardinality() > 0) {
		//Get the last-indexed (i.e., highest-cost) row:
		unsigned int row_ind;
		initial_greedy_solution_bitmap.select(initial_greedy_solution_bitmap.cardinality() - 1, & row_ind);
		set_cover_row row = rows[row_ind];
		//Check if it is redundant (i.e., if the other rows still in the reduced solution set can cover the original target set):
		reduced_greedy_solution_candidates.erase(row_ind);
		Roaring row_union = Roaring();
		for (unsigned int other_row_ind : reduced_greedy_solution_candidates) {
			set_cover_row other_row = rows[other_row_ind];
			row_union |= other_row.bits;
		}
		if (!target.isSubset(row_union)) {
			reduced_greedy_solution_candidates.insert(row_ind);
			reduced_greedy_solution.rows.push_front(row);
			reduced_greedy_solution.cost += row.cost;
		}
		initial_greedy_solution_bitmap.remove(row_ind);
	}
	return reduced_greedy_solution;
}

/**
 * Given a vector of row data structures, and a target bitmap to cover,
 * populates the given solution list with progressively best-found solutions via branch and bound.
 * Optionally, a fixed upper bound can be specified, and all solutions with costs within this bound will be enumerated.
 */
void branch_and_bound(vector<set_cover_row> rows, Roaring target, list<set_cover_solution> & solutions, int fixed_ub=-1) {
	//If no upper bound is specified, then obtain a good initial upper bound quickly using the trivial solution and the greedy solution:
	int ub = fixed_ub;
	if (fixed_ub < 0) {
		set_cover_solution trivial_solution = get_trivial_solution(rows, target);
		set_cover_solution greedy_solution = get_greedy_solution(rows, target);
		ub = min(trivial_solution.cost, greedy_solution.cost);
	}
	//Initialize three bitmaps that partition the row set into rows that are accepted in the current candidate solution,
	//rows that are rejected from the current candidate solution,
	//and rows that have not been processed yet:
	Roaring accepted = Roaring();
	Roaring rejected = Roaring();
	Roaring remaining = Roaring();
	remaining.addRange(0, rows.size());
	//Initialize a stack of branch and bound nodes:
	stack<branch_and_bound_node> nodes = stack<branch_and_bound_node>();
	branch(rows, target, accepted, remaining, nodes);
	//Then continue with branch and bound until there is nothing left to be processed:
	while (!nodes.empty()) {
		//Get the current node from the stack:
		branch_and_bound_node & node = nodes.top();
		//Adjust the set partitions to reflect the candidate solution representing by the current node:
		unsigned int candidate_row = node.candidate_row;
		if (node.state == node_state::ACCEPT) {
			//Add the candidate row to the solution:
			remaining.remove(candidate_row);
			accepted.add(candidate_row);
			//Update its state:
			node.state = node_state::REJECT;
		}
		else if (node.state == node_state::REJECT) {
			//Exclude the candidate row from the solution:
			accepted.remove(candidate_row);
			rejected.add(candidate_row);
			//Update its state:
			node.state = node_state::DONE;
		}
		else {
			//We're done processing this node, and we can move on:
			rejected.remove(candidate_row);
			remaining.add(candidate_row);
			nodes.pop();
			continue;
		}
		//Lower-bound the cost of any solution containing this one:
		int lb = bound(rows, accepted);
		//Check if it is a feasible solution:
		if (is_feasible(rows, target, accepted)) {
			//If it is, then the lower bound we calculated is this solution's cost; check if it is within the current upper bound:
			if (lb <= ub) {
				//If it is within the upper bound, then add a solution to the list;
				//if the upper bound is not fixed, then update the upper bound, as well:
				if (fixed_ub > 0) {
					ub = lb;
				}
				set_cover_solution solution;
				solution.rows = list<set_cover_row>();
				for (Roaring::const_iterator row_ind = accepted.begin(); row_ind != accepted.end(); row_ind++) {
					set_cover_row row = rows[* row_ind];
					solution.rows.push_back(row);
				}
				solution.cost = lb;
				solutions.push_back(solution);
			}
		}
		//If it is not, then check if there is any feasible solution that contains this one:
		else if (is_any_branch_feasible(rows, target, accepted, remaining)) {
			//If there is, then check if it could have a cost within the upper bound:
			if (lb <= ub) {
				//If so, then branch on this node:
				branch(rows, target, accepted, remaining, nodes);
			}
		}
	}
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
	int fixed_ub = -1;
	const char* const short_opts = "ht:b:";
	const option long_opts[] = {
		{"split", no_argument, & split, 1},
		{"orth", no_argument, & orth, 1},
		{"def", no_argument, & def, 1},
		{"threshold", required_argument, nullptr, 't'},
		{"bound", required_argument, nullptr, 'b'},
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
			case 'b':
				fixed_ub = atoi(optarg);
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
	cout << "Finding optimal substemmata for witness " << primary_wit_id << "..." << endl;
	//Populate a set of IDs for all other witnesses that match the input parameters
	//and a map of the witnesses themselves, keyed by ID:
	unordered_set<string> secondary_wit_ids = unordered_set<string>();
	list<witness> secondary_witnesses = list<witness>();
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
		secondary_witnesses.push_back(secondary_wit);
	}
	//Initialize the primary witness relative to all of the secondary witnesses in this map:
	secondary_wit_ids.insert(primary_wit_id);
	witness primary_wit = witness(primary_wit_id, secondary_wit_ids, app);
	//Now populate the primary witness's list of potential ancestors:
	primary_wit.set_potential_ancestor_ids(secondary_witnesses);
	//Using this list, populate a vector of set cover rows:
	vector<set_cover_row> rows = vector<set_cover_row>();
	for (string secondary_wit_id : primary_wit.get_potential_ancestor_ids()) {
		set_cover_row row;
		row.id = secondary_wit_id;
		row.bits = primary_wit.get_explained_readings_for_witness(secondary_wit_id);
		row.cost = (primary_wit.get_explained_readings_for_witness(primary_wit_id) ^ primary_wit.get_agreements_for_witness(secondary_wit_id)).cardinality();
		rows.push_back(row);
	}
	//Likewise, populate a vector of bitmaps representing the columns of the covering matrix:
	vector<Roaring> cols = vector<Roaring>();
	for (variation_unit vu : app.get_variation_units()) {
		Roaring col = Roaring();
		cols.push_back(col);
	}
	unsigned int row_ind = 0;
	for (set_cover_row row : rows) {
		for (Roaring::const_iterator col_ind = row.bits.begin(); col_ind != row.bits.end(); col_ind++) {
			cols[* col_ind].add(row_ind);
		}
		row_ind++;
	}
	//Initialize the bitmap of the target set to be covered:
	Roaring target = primary_wit.get_explained_readings_for_witness(primary_wit_id);
	//If any column in the target set is not covered by any row, then there is no solution:
	unordered_set<unsigned int> uncovered = unordered_set<unsigned int>();
	for (Roaring::const_iterator col_ind = target.begin(); col_ind != target.end(); col_ind++) {
		if (cols[* col_ind].cardinality() == 0) {
			uncovered.insert(* col_ind);
		}
	}
	if (uncovered.size() > 0) {
		cout << "The witness with ID " << primary_wit_id << " cannot be explained by any of its potential ancestors at the following variation units: ";
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
	Roaring unique_row_inds = Roaring();
	for (Roaring::const_iterator col_ind = target.begin(); col_ind != target.end(); col_ind++) {
		if (cols[* col_ind].cardinality() == 1) {
			unsigned int row_ind;
			cols[* col_ind].select(0, & row_ind);
			unique_row_inds.add(row_ind);
		}
	}
	list<set_cover_row> unique_rows = list<set_cover_row>();
	for (Roaring::const_iterator row_ind = unique_row_inds.begin(); row_ind != unique_row_inds.end(); row_ind++) {
		set_cover_row row = rows[*row_ind];
		unique_rows.push_back(row);
		target ^= target & row.bits;
	}
	//If the remaining set of columns to be covered is empty, then the rows with unique column coverage
	//represent the witnesses in the only valid substemma for the primary witness:
	if (target.cardinality() == 0) {
		set_cover_solution solution;
		solution.cost = 0;
		for (set_cover_row row : unique_rows) {
			solution.rows.push_back(row);
			solution.cost += row.cost;
		}
		//If a fixed upper bound was specified and the cost of this solution is higher than it,
		//then report to the user that there are no solutions within the bound:
		if (fixed_ub >= 0 && solution.cost > fixed_ub) {
			printf("No substemma exists with a cost below %i; try again with a higher bound.\n", fixed_ub);
			return 0;
		}
		list<set_cover_solution> solutions = list<set_cover_solution>({solution});
		print_substemmata(solutions);
		return 0;
	}
	//Otherwise, populate a new vector with just the rows that overlap with any of the remaining columns:
	vector<set_cover_row> subproblem_rows = vector<set_cover_row>();
	for (set_cover_row row : rows) {
		if ((row.bits & target).cardinality() > 0) {
			subproblem_rows.push_back(row);
		}
	}
	//If a fixed upper bound was specified, then update it based on the costs of the unique coverage rows:
	if (fixed_ub >= 0) {
		for (set_cover_row row : unique_rows) {
			fixed_ub -= row.cost;
		}
	}
	//Then populate a list of best-found solutions for the resulting subproblem using branch and bound:
	list<set_cover_solution> solutions = list<set_cover_solution>();
	branch_and_bound(rows, target, solutions, fixed_ub);
	//If a fixed upper bound was specified and no solutions were found within the bound,
	//then report this to the user:
	if (fixed_ub >= 0 && solutions.empty()) {
		printf("No substemma exists with a cost below %i; try again with a higher bound.\n", fixed_ub);
		return 0;
	}
	//Otherwise, sort the solutions in order of their costs, then cardinalities:
	solutions.sort([](const set_cover_solution & s1, const set_cover_solution & s2) {
		return s1.cost < s2.cost ? -1 : (s1.cost > s2.cost ? 1 : (s1.rows.size() < s2.rows.size() ? -1 : (s1.rows.size() > s2.rows.size() ? 1 : 0)));
	});
	//Then, for each of the best-found solutions for this subproblem, add back in the unique coverage rows found earlier:
	for (set_cover_solution & solution : solutions) {
		for (set_cover_row row : unique_rows) {
			solution.rows.push_front(row);
			solution.cost += row.cost;
		}
	}
	//Then print the solutions and their costs:
	print_substemmata(solutions);
	return 0;
}
