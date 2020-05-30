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
#include "sqlite3.h"
#include "roaring.hh"
#include "witness.h"
#include "apparatus.h"
#include "variation_unit.h"
#include "set_cover_solver.h"

using namespace std;

/**
 * Determines if the WITNESSES table of the given SQLite database
 * contains a row with the given witness ID.
 */
bool witness_id_exists(sqlite3 * input_db, const string & wit_id) {
	bool witness_id_exists = false;
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_witnesses_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM WITNESSES WHERE WITNESS=?", -1, & select_from_witnesses_stmt, 0);
	sqlite3_bind_text(select_from_witnesses_stmt, 1, wit_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_witnesses_stmt);
	witness_id_exists = (rc == SQLITE_ROW) ? true : false;
	sqlite3_finalize(select_from_witnesses_stmt);
	return witness_id_exists;
}

/**
 * Retrieves all rows from the WITNESSES table of the given SQLite database
 * and returns a list of witness IDs populated with its contents.
 */
list<string> get_witness_ids(sqlite3 * input_db) {
	list<string> witness_ids = list<string>();
	int rc; //to store SQLite macros
	cout << "Retrieving witness IDs..." << endl;
	sqlite3_stmt * select_from_witnesses_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM WITNESSES ORDER BY ROWID", -1, & select_from_witnesses_stmt, 0);
	rc = sqlite3_step(select_from_witnesses_stmt);
	while (rc == SQLITE_ROW) {
		string wit_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_witnesses_stmt, 0)));
		witness_ids.push_back(wit_id);
		rc = sqlite3_step(select_from_witnesses_stmt);
	}
	sqlite3_finalize(select_from_witnesses_stmt);
	return witness_ids;
}

/**
 * Retrieves rows relative to the given primary witness ID from the GENEALOGICAL_COMPARISONS table of the given SQLite database
 * and returns a map of genealogical comparisons populated with its contents.
 */
unordered_map<string, genealogical_comparison> get_primary_witness_genealogical_comparisons(sqlite3 * input_db, const string & _primary_wit_id) {
	unordered_map<string, genealogical_comparison> primary_witness_genealogical_comparisons = unordered_map<string, genealogical_comparison>();
	int rc; //to store SQLite macros
	cout << "Retrieving genealogical comparisons relative to primary witness " << _primary_wit_id << "..." << endl;
	sqlite3_stmt * select_from_genealogical_comparisons_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM GENEALOGICAL_COMPARISONS WHERE PRIMARY_WIT=?", -1, & select_from_genealogical_comparisons_stmt, 0);
	sqlite3_bind_text(select_from_genealogical_comparisons_stmt, 1, _primary_wit_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_genealogical_comparisons_stmt);
	while (rc == SQLITE_ROW) {
		genealogical_comparison comp;
		string primary_wit_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_genealogical_comparisons_stmt, 0)));
		string secondary_wit_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_genealogical_comparisons_stmt, 1)));
		int agreements_bytes = sqlite3_column_bytes(select_from_genealogical_comparisons_stmt, 2);
		const char * agreements_buf = reinterpret_cast<const char *>(sqlite3_column_blob(select_from_genealogical_comparisons_stmt, 2));
		Roaring agreements = Roaring::readSafe(agreements_buf, agreements_bytes);
		comp.agreements = agreements;
		int explained_bytes = sqlite3_column_bytes(select_from_genealogical_comparisons_stmt, 3);
		const char * explained_buf = reinterpret_cast<const char *>(sqlite3_column_blob(select_from_genealogical_comparisons_stmt, 3));
		Roaring explained = Roaring::readSafe(explained_buf, explained_bytes);
		comp.explained = explained;
		float cost = float(sqlite3_column_double(select_from_genealogical_comparisons_stmt, 4));
		comp.cost = cost;
		primary_witness_genealogical_comparisons[secondary_wit_id] = comp;
		rc = sqlite3_step(select_from_genealogical_comparisons_stmt);
	}
	sqlite3_finalize(select_from_genealogical_comparisons_stmt);
	return primary_witness_genealogical_comparisons;
}

/**
 * Retrieves rows corresponding to the each secondary witness's genealogical comparisons relative to itself and the given primary witness
 * from the GENEALOGICAL_COMPARISONS table of the given SQLite database
 * and returns a map of genealogical comparison maps populated with its contents.
 */
unordered_map<string, unordered_map<string, genealogical_comparison>> get_secondary_witness_genealogical_comparisons(sqlite3 * input_db, const string & _primary_wit_id) {
	unordered_map<string, unordered_map<string, genealogical_comparison>> secondary_witness_genealogical_comparisons = unordered_map<string, unordered_map<string, genealogical_comparison>>();
	int rc; //to store SQLite macros
	cout << "Retrieving genealogical comparisons relative to secondary witnesses..." << endl;
	sqlite3_stmt * select_from_genealogical_comparisons_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM GENEALOGICAL_COMPARISONS WHERE (PRIMARY_WIT=SECONDARY_WIT) <> (SECONDARY_WIT=?)", -1, & select_from_genealogical_comparisons_stmt, 0);
	sqlite3_bind_text(select_from_genealogical_comparisons_stmt, 1, _primary_wit_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_genealogical_comparisons_stmt);
	while (rc == SQLITE_ROW) {
		genealogical_comparison comp;
		string primary_wit_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_genealogical_comparisons_stmt, 0)));
		//Check if there is already a comparison map for this witness in the output map, and create one if not:
		if (secondary_witness_genealogical_comparisons.find(primary_wit_id) == secondary_witness_genealogical_comparisons.end()) {
			secondary_witness_genealogical_comparisons[primary_wit_id] = unordered_map<string, genealogical_comparison>();
		}
		string secondary_wit_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_genealogical_comparisons_stmt, 1)));
		int agreements_bytes = sqlite3_column_bytes(select_from_genealogical_comparisons_stmt, 2);
		const char * agreements_buf = reinterpret_cast<const char *>(sqlite3_column_blob(select_from_genealogical_comparisons_stmt, 2));
		Roaring agreements = Roaring::readSafe(agreements_buf, agreements_bytes);
		comp.agreements = agreements;
		int explained_bytes = sqlite3_column_bytes(select_from_genealogical_comparisons_stmt, 3);
		const char * explained_buf = reinterpret_cast<const char *>(sqlite3_column_blob(select_from_genealogical_comparisons_stmt, 3));
		Roaring explained = Roaring::readSafe(explained_buf, explained_bytes);
		comp.explained = explained;
		float cost = float(sqlite3_column_double(select_from_genealogical_comparisons_stmt, 4));
		comp.cost = cost;
		secondary_witness_genealogical_comparisons.at(primary_wit_id)[secondary_wit_id] = comp;
		rc = sqlite3_step(select_from_genealogical_comparisons_stmt);
	}
	sqlite3_finalize(select_from_genealogical_comparisons_stmt);
	return secondary_witness_genealogical_comparisons;
}

/**
 * Retrieves all rows from the VARIATION_UNITS table of the given SQLite database
 * and returns a vector of variation_unit labels populated with its contents.
 */
vector<string> get_variation_unit_labels(sqlite3 * input_db) {
	vector<string> variation_unit_labels = vector<string>();
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_variation_units_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM VARIATION_UNITS ORDER BY ROWID", -1, & select_from_variation_units_stmt, 0);
	rc = sqlite3_step(select_from_variation_units_stmt);
	while (rc == SQLITE_ROW) {
		string vu_label = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_variation_units_stmt, 1)));
		variation_unit_labels.push_back(vu_label);
		rc = sqlite3_step(select_from_variation_units_stmt);
	}
	sqlite3_finalize(select_from_variation_units_stmt);
	return variation_unit_labels;
}

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
	float fixed_ub = numeric_limits<float>::infinity();
	string input_db_name = string();
	string primary_wit_id = string();
	try {
		cxxopts::Options options("optimize_substemmata", "Get a table of best-found substemmata for the witness with the given ID.");
		options.custom_help("[-h] [-b bound] input_db witness");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("b,bound", "fixed upper bound on substemmata cost; if specified, list all substemmata with costs within this bound", cxxopts::value<float>());
		options.add_options("positional")
				("input_db", "genealogical cache database", cxxopts::value<string>())
				("witness", "ID of the witness whose relatives are desired, as found in its <witness> element in the XML file", cxxopts::value<vector<string>>());
		options.parse_positional({"input_db", "witness"});
		auto args = options.parse(argc, argv);
		//Print help documentation and exit if specified:
		if (args.count("help")) {
			cout << options.help({""}) << endl;
			exit(0);
		}
		//Parse the optional arguments:
		if (args.count("b")) {
			fixed_ub = args["b"].as<float>();
		}
		//Parse the positional arguments:
		if (!args.count("input_db") || args.count("witness") != 1) {
			cerr << "Error: 2 positional arguments (input_db and witness) are required." << endl;
			exit(1);
		}
		else {
			input_db_name = args["input_db"].as<string>();
			primary_wit_id = args["witness"].as<vector<string>>()[0];
		}
	}
	catch (const cxxopts::OptionException & e) {
		cerr << "Error parsing options: " << e.what() << endl;
		exit(-1);
	}
	//Open the database:
	cout << "Opening database..." << endl;
	sqlite3 * input_db;
	int rc = sqlite3_open(input_db_name.c_str(), & input_db);
	if (rc) {
		cerr << "Error opening database " << input_db_name << ": " << sqlite3_errmsg(input_db) << endl;
		exit(1);
	}
	//Check if the primary witness ID is in the database:
	bool primary_wit_exists = witness_id_exists(input_db, primary_wit_id);
	if (!primary_wit_exists) {
		cerr << "Error: The WITNESSES table has no rows with WITNESS = " << primary_wit_id << "." << endl;
		exit(1);
	}
	//Retrieve a list of all witness IDs:
	list<string> list_wit = get_witness_ids(input_db);
	//Retrieve all necessary genealogical comparisons relative to the primary witness:
	unordered_map<string, genealogical_comparison> primary_witness_genealogical_comparisons = get_primary_witness_genealogical_comparisons(input_db, primary_wit_id);
	//Retrieve all necessary genealogical comparisons relative to the secondary witnesses:
	unordered_map<string, unordered_map<string, genealogical_comparison>> secondary_witness_genealogical_comparisons = get_secondary_witness_genealogical_comparisons(input_db, primary_wit_id);
	//Initialize the primary witness:
	witness primary_wit = witness(primary_wit_id, primary_witness_genealogical_comparisons);
	//Then initialize a list of all witnesses:
	list<witness> witnesses = list<witness>();
	for (string wit_id : list_wit) {
		//The primary witness is initialized using genealogical comparisons with all witnesses:
		if (wit_id == primary_wit_id) {
			witness wit = witness(wit_id, primary_witness_genealogical_comparisons);
			witnesses.push_back(wit);
		}
		//Secondary witnesses are initialized using genealogical comparisons with just themselves and the primary witness:
		else {
			unordered_map<string, genealogical_comparison> genealogical_comparisons = secondary_witness_genealogical_comparisons.at(wit_id);
			witness wit = witness(wit_id, genealogical_comparisons);
			witnesses.push_back(wit);
		}
	}
	//Then populate the primary witness's list of potential ancestors:
	primary_wit.set_potential_ancestor_ids(witnesses);
	//If the primary witness has no potential ancestors, then let the user know:
	if (primary_wit.get_potential_ancestor_ids().empty()) {
		cout << "The witness with ID " << primary_wit_id << " has no potential ancestors. This may be because it is too fragmentary or because it has equal priority to the Ausgangstext according to local stemmata." << endl;
		exit(0);
	}
	if (fixed_ub == numeric_limits<float>::infinity()) {
		cout << "Finding optimal substemmata for witness " << primary_wit_id << "..." << endl;
	}
	else {
		cout << "Finding all substemmata for witness " << primary_wit_id << " with costs within " << fixed_ub << "..." << endl;
	}
	//Populate a vector of set cover rows using genealogical comparisons for the primary witness's potential ancestors:
	vector<set_cover_row> rows = vector<set_cover_row>();
	for (string wit_id : primary_wit.get_potential_ancestor_ids()) {
		genealogical_comparison comp = primary_wit.get_genealogical_comparison_for_witness(wit_id);
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
			vector<string> vu_labels = get_variation_unit_labels(input_db);
			for (Roaring::const_iterator it = uncovered_columns.begin(); it != uncovered_columns.end(); it++) {
				unsigned int col_ind = *it;
				string vu_label = vu_labels[col_ind];
				if (it != uncovered_columns.begin()) {
					cout << ", ";
				}
				cout << vu_label;
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
	//Close the database:
	cout << "Closing database..." << endl;
	sqlite3_close(input_db);
	cout << "Database closed." << endl;
	//Otherwise, print the solutions and their costs:
	print_substemmata(primary_wit_id, solutions);
	exit(0);
}
