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
#include <set>
#include <unordered_map>
#include <limits>

#include "cxxopts.h"
#include "pugixml.h"
#include "roaring.hh"
#include "witness.h"
#include "apparatus.h"
#include "variation_unit.h"
#include "set_cover_solver.h"

using namespace std;

/**
 * Given primary witness ID and a list of set cover solutions (assumed to be sorted from lowest cost to highest),
 * prints the corresponding best-found substemmata for the primary witness, along with their costs and number of agreements with the primary witness.
 */
void print_substemmata(const string & primary_wit_id, const list<set_cover_solution> & solutions) {
	//Print the caption:
	cout << "Optimal substemmata for witness W1 = " << primary_wit_id << ":\n\n";
	//Print the header row:
	cout << std::left << std::setw(48) << "SUBSTEMMA";
	cout << std::right << std::setw(8) << "COST";
	cout << std::right << std::setw(8) << "AGREE";
	cout << "\n\n";
	//Print the subsequent rows:
	for (set_cover_solution solution : solutions) {
		string solution_str = "";
		for (set_cover_row row : solution.rows) {
			if (row.id != solution.rows.front().id) {
				solution_str += ", ";
			}
			solution_str += row.id;
		}
		cout << std::left << std::setw(48) << solution_str;
		cout << std::right << std::setw(8) << solution.cost;
		cout << std::right << std::setw(8) << solution.agreements;
		cout << "\n";
	}
	cout << endl;
	return;
}

/**
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Read in the command-line options:
	int threshold = 0;
	float fixed_ub = numeric_limits<float>::infinity();
	set<string> trivial_reading_types = set<string>();
	bool merge_splits = false;
	string input_xml = string();
	string primary_wit_id = string();
	try {
		cxxopts::Options options("optimize_substemmata", "Get a table of best-found substemmata for the witness with the given ID.");
		options.custom_help("[-h] [-t threshold] [-b bound] [-z trivial_reading_types] [--merge-splits] input_xml witness");
		//options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("t,threshold", "minimum extant readings threshold", cxxopts::value<int>())
				("b,bound", "fixed upper bound on substemmata cost; if specified, list all substemmata with costs within this bound", cxxopts::value<float>())
				("z", "space-separated list of reading types to treat as trivial (e.g., defective orthographic)", cxxopts::value<vector<string>>())
				("merge-splits", "merge split attestations of the same reading", cxxopts::value<bool>());
		options.add_options("positional")
				("input_xml", "collation file in TEI XML format", cxxopts::value<string>())
				("witness", "ID of the witness whose relatives are desired, as found in its <witness> element in the XML file", cxxopts::value<vector<string>>());
		options.parse_positional({"input_xml", "witness"});
		auto args = options.parse(argc, argv);
		//Print help documentation and exit if specified:
		if (args.count("help")) {
			cout << options.help({""}) << endl;
			exit(0);
		}
		//Parse the optional arguments:
		if (args.count("t")) {
			threshold = args["t"].as<int>();
		}
		if (args.count("b")) {
			fixed_ub = args["b"].as<float>();
		}
		if (args.count("z")) {
			for (string trivial_reading_type : args["z"].as<vector<string>>()) {
				trivial_reading_types.insert(trivial_reading_type);
			}
		}
		if (args.count("merge-splits")) {
			merge_splits = args["merge-splits"].as<bool>();
		}
		//Parse the positional arguments:
		if (!args.count("input_xml") || args.count("witness") != 1) {
			cerr << "Error: 2 positional arguments (input_xml and witness) are required." << endl;
			exit(1);
		}
		else {
			input_xml = args["input_xml"].as<string>();
			primary_wit_id = args["witness"].as<vector<string>>()[0];
		}
	}
	catch (const cxxopts::OptionException & e) {
		cerr << "Error parsing options: " << e.what() << endl;
		exit(-1);
	}
	//Attempt to parse the input XML file as an apparatus:
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(input_xml.c_str());
	if (!result) {
		cerr << "Error: An error occurred while loading XML file " << input_xml << ": " << result.description() << endl;
		exit(1);
	}
	pugi::xml_node tei_node = doc.child("TEI");
	if (!tei_node) {
		cerr << "Error: The XML file " << input_xml << " does not have a <TEI> element as its root element." << endl;
		exit(1);
	}
	apparatus app = apparatus(tei_node, merge_splits, trivial_reading_types);
	//Ensure that the primary witness is included in the apparatus's <listWit> element:
	bool primary_wit_exists = false;
	for (string wit_id : app.get_list_wit()) {
		if (wit_id == primary_wit_id) {
			primary_wit_exists = true;
			break;
		}
	}
	if (!primary_wit_exists) {
		cerr << "Error: The XML file's <listWit> element has no child <witness> element with ID " << primary_wit_id << "." << endl;
		exit(1);
	}
	//If the user has specified a minimum extant readings threshold,
	//then populate a list of witnesses that meet the threshold:
	list<string> list_wit = list<string>();
	if (threshold > 0) {
		cout << "Filtering out fragmentary witnesses... " << endl;
		for (string wit_id : app.get_list_wit()) {
			if (app.get_extant_passages_for_witness(wit_id) >= threshold) {
				list_wit.push_back(wit_id);
			}
			//If the primary witness is fragmentary, then report this to the user and exit:
			else if (wit_id == primary_wit_id) {
				cout << "Primary witness " << primary_wit_id << " does not meet the specified minimum extant readings threshold of " << threshold << "." << endl;
				exit(0);
			}
		}
	}
	//Otherwise, just use the full list of witnesses found in the apparatus:
	else {
		list_wit = app.get_list_wit();
	}
	cout << "Calculating genealogical relationships between witness " << primary_wit_id << " and all other witnesses..." << endl;
	//Then initialize the primary witness:
	witness primary_wit = witness(primary_wit_id, list_wit, app);
	//Then populate a list of secondary witnesses, keyed by ID:
	list<witness> secondary_witnesses = list<witness>();
	for (string secondary_wit_id : list_wit) {
		//Skip the primary witness:
		if (secondary_wit_id == primary_wit_id) {
			continue;
		}
		//Initialize the secondary witness relative to the primary witness:
		list<string> secondary_list_wit = list<string>({primary_wit_id, secondary_wit_id});
		witness secondary_wit = witness(secondary_wit_id, secondary_list_wit, app);
		//Add it to the list:
		secondary_witnesses.push_back(secondary_wit);
	}
	if (fixed_ub == numeric_limits<float>::infinity()) {
		cout << "Finding optimal substemmata for witness " << primary_wit_id << "..." << endl;
	}
	else {
		cout << "Finding all substemmata for witness " << primary_wit_id << " with costs within " << fixed_ub << "..." << endl;
	}
	//Populate the primary witness's list of potential ancestors:
	primary_wit.set_potential_ancestor_ids(secondary_witnesses);
	//If this witness has no potential ancestors, then let the user know:
	if (primary_wit.get_potential_ancestor_ids().empty()) {
		cout << "The witness with ID " << primary_wit_id << " has no potential ancestors. This may be because it is too fragmentary or because it has equal priority to the Ausgangstext according to local stemmata." << endl;
		exit(0);
	}
	//Using this list, populate a vector of set cover rows:
	vector<set_cover_row> rows = vector<set_cover_row>();
	for (string secondary_wit_id : primary_wit.get_potential_ancestor_ids()) {
		genealogical_comparison comp = primary_wit.get_genealogical_comparison_for_witness(secondary_wit_id);
		set_cover_row row;
		row.id = secondary_wit_id;
		row.agreements = comp.agreements;
		row.explained = comp.explained;
		row.cost = comp.cost;
		rows.push_back(row);
	}
	//Sort this vector by increasing cost and decreasing number of agreements:
	sort(begin(rows), end(rows), [](const set_cover_row & r1, const set_cover_row & r2) {
		return r1.cost < r2.cost ? true : (r1.cost > r2.cost ? false : (r1.agreements.cardinality() > r2.agreements.cardinality()));
	});
	//Initialize the bitmap of the target set to be covered:
	Roaring target = primary_wit.get_genealogical_comparison_for_witness(primary_wit_id).explained;
	//Initialize the list of solutions to be populated:
	list<set_cover_solution> solutions;
	//Then populate it using the solver:
	set_cover_solver solver = fixed_ub < numeric_limits<float>::infinity() ? set_cover_solver(rows, target, fixed_ub) : set_cover_solver(rows, target);
	solver.solve(solutions);
	//If the solution set is empty, then find out why:
	if (solutions.empty()) {
		//If the set cover problem is infeasible, then inform the user of the variation units corresponding to the uncovered columns:
		Roaring uncovered_columns = solver.get_uncovered_columns();
		if (!uncovered_columns.isEmpty()) {
			cout << "The witness with ID " << primary_wit_id << " cannot be explained by any of its potential ancestors at the following variation units: ";
			vector<variation_unit> vus = app.get_variation_units();
			for (Roaring::const_iterator it = uncovered_columns.begin(); it != uncovered_columns.end(); it++) {
				unsigned int col_ind = *it;
				variation_unit vu = vus[col_ind];
				if (it != uncovered_columns.begin()) {
					cout << ", ";
				}
				cout << vu.get_label();
			}
			cout << endl;
			exit(0);
		}
		//If a fixed upper bound was specified, and no solution was found, then tell the user to raise the upper bound:
		if (fixed_ub < numeric_limits<float>::infinity()) {
			cout << "No substemma exists with a cost below " << fixed_ub << "; try again with a higher bound or without specifying a fixed upper bound." << endl;
			cout << "Alternatively, if any local stemma is not connected, then it may be the source of an unexplained reading in this witness." << endl;
			exit(0);
		}
		//Otherwise, no solution was found because the given witness's potential ancestors cannot explain all of its readings:
		else {
			cout << "No substemma exists; if any local stemma is not connected, then it may be the source of an unexplained reading in this witness." << endl;
			exit(0);
		}
	}
	//Otherwise, print the solutions and their costs:
	print_substemmata(primary_wit_id, solutions);
	exit(0);
}
