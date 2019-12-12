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
#include <getopt.h>

#include "pugixml.h"
#include "roaring.hh"
#include "witness.h"
#include "apparatus.h"
#include "variation_unit.h"
#include "set_cover_solver.h"

using namespace std;

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
	//If this witness has no potential ancestors, then let the user know:
	if (primary_wit.get_potential_ancestor_ids().empty()) {
		cout << "The witness with ID " << primary_wit_id << " has no potential ancestors. This may be because it is too fragmentary or because it has equal priority to the Ausgangstext according to local stemmata." << endl;
		exit(0);
	}
	//Using this list, populate a vector of set cover rows:
	vector<set_cover_row> rows = vector<set_cover_row>();
	for (string secondary_wit_id : primary_wit.get_potential_ancestor_ids()) {
		set_cover_row row;
		row.id = secondary_wit_id;
		row.bits = primary_wit.get_explained_readings_for_witness(secondary_wit_id);
		row.cost = (primary_wit.get_explained_readings_for_witness(primary_wit_id) ^ primary_wit.get_agreements_for_witness(secondary_wit_id)).cardinality();
		rows.push_back(row);
	}
	//Initialize the bitmap of the target set to be covered:
	Roaring target = primary_wit.get_explained_readings_for_witness(primary_wit_id);
	//Initialize the list of solutions to be populated:
	list<set_cover_solution> solutions;
	//Then populate it using the solver:
	set_cover_solver solver = fixed_ub >= 0 ? set_cover_solver(rows, target, fixed_ub) : set_cover_solver(rows, target);
	solver.solve(solutions);
	//If the solution set is empty, then find out why:
	if (solutions.empty()) {
		//If the set cover problem is infeasible, then inform the user of the variation units corresponding to the uncovered columns:
		Roaring uncovered_columns = solver.get_uncovered_columns();
		if (uncovered_columns.cardinality() > 0) {
			cout << "The witness with ID " << primary_wit_id << " cannot be explained by any of its potential ancestors at the following variation units: ";
			unsigned int vu_ind = 0;
			for (variation_unit vu : app.get_variation_units()) {
				if (uncovered_columns.contains(vu_ind)) {
					cout << vu.get_label() << " ";
				}
				vu_ind++;
			}
			cout << endl;
			exit(0);
		}
		//If a fixed upper bound was specified, and no solution was found, then tell the user to raise the upper bound:
		if (fixed_ub >= 0) {
			cout << "No substemma exists with a cost below " << fixed_ub << "; try again with a higher bound or without specifying a fixed upper bound." << endl;
			exit(0);
		}
	}
	//Otherwise, print the solutions and their costs:
	print_substemmata(solutions);
	exit(0);
}
