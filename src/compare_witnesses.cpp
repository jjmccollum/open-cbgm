/*
 * compare_witnesses.cpp
 *
 *  Created on: Nov 20, 2019
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

using namespace std;

/**
 * Data structure representing a comparison between a primary witness and a secondary witness.
 */
struct witness_comparison {
	string id; //ID of the secondary witness
	int dir; //-1 if primary witness is prior; 1 if posterior; 0 otherwise
	int nr; //rank of the secondary witness as a potential ancestor of the primary witness
	int pass; //number of variation units where the primary witness is extant
	float perc; //percentage of agreement in variation units where the primary witness is extant
	int eq; //number of agreements in variation units where the primary witness is extant
	int prior; //number of variation units where the primary witness has a prior reading
	int posterior; //number of variation units where the primary witness has a posterior reading
	//int uncl; //number of variation units where both witnesses have different readings, but either's could explain that of the other
	int norel; //number of variation units where the primary witness has a reading unrelated to that of the secondary witness
	float cost; //genealogical cost of relationship from the target witnesses if it is a potential ancestor
};

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
 * Given a primary witness ID, a set of desired secondary witnesses to filter on,
 * and a list of witness comparisons (assumed to be sorted in decreasing order of agreements),
 * prints a list of genealogical comparisons relative to the primary witness.
 */
void print_comparisons(const string & primary_wit_id, const set<string> & secondary_wit_ids, const list<witness_comparison> & comparisons) {
	//Print the caption:
	cout << "Genealogical comparisons for W1 = " << primary_wit_id << ":";
	cout << "\n\n";
	//Print the header row:
	cout << std::left << std::setw(8) << "W2";
	cout << std::left << std::setw(4) << "DIR";
	cout << std::right << std::setw(4) << "NR";
	cout << std::right << std::setw(8) << "PASS";
	cout << std::right << std::setw(8) << "EQ";
	cout << std::right << std::setw(12) << ""; //percentage of agreements among mutually extant passages
	cout << std::right << std::setw(8) << "W1>W2";
	cout << std::right << std::setw(8) << "W1<W2";
	cout << std::right << std::setw(8) << "NOREL";
	cout << std::right << std::setw(12) << "COST";
	cout << "\n\n";
	//Print the subsequent rows:
	for (witness_comparison comparison : comparisons) {
		//If the filter set is not empty, then only print rows for witness IDs in that set:
		if (!secondary_wit_ids.empty() && secondary_wit_ids.find(comparison.id) == secondary_wit_ids.end()) {
			continue;
		}
		cout << std::left << std::setw(8) << comparison.id;
		cout << std::left << std::setw(4) << (comparison.dir == -1 ? "<" : (comparison.dir == 1 ? ">" : "="));
		cout << std::right << std::setw(4) << (comparison.nr > 0 ? to_string(comparison.nr) : "");
		cout << std::right << std::setw(8) << comparison.pass;
		cout << std::right << std::setw(8) << comparison.eq;
		cout << std::right << std::setw(3) << "(" << std::setw(7) << fixed << std::setprecision(3) << comparison.perc << std::setw(2) << "%)";
		cout << std::right << std::setw(8) << comparison.prior;
		cout << std::right << std::setw(8) << comparison.posterior;
		cout << std::right << std::setw(8) << comparison.norel;
		if (comparison.cost >= 0) {
			cout << std::right << std::setw(12) << fixed << std::setprecision(3) << comparison.cost;
		}
		else {
			cout << std::right << std::setw(12) << "";
		}
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
	string input_db_name = string();
	string primary_wit_id = string();
	set<string> secondary_wit_ids = set<string>();
	list<string> secondary_wit_ids_ordered = list<string>(); //for printing purposes
	try {
		cxxopts::Options options("compare_witnesses", "Get a table of genealogical relationships between the witness with the given ID and other witnesses, as specified by the user.");
		options.custom_help("[-h] input_db witness [secondary_witnesses]");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help");
		options.add_options("positional")
				("input_db", "genealogical cache database", cxxopts::value<string>())
				("witness", "ID of the primary witness to be compared, as found in its <witness> element in the XML file", cxxopts::value<string>())
				("secondary_witnesses", "IDs of secondary witness to be compared to the primary witness (if not specified, then the primary witness will be compared to all other witnesses)", cxxopts::value<vector<string>>());
		options.parse_positional({"input_db", "witness", "secondary_witnesses"});
		auto args = options.parse(argc, argv);
		//Print help documentation and exit if specified:
		if (args.count("help")) {
			cout << options.help({""}) << endl;
			exit(0);
		}
		//Parse the positional arguments:
		if (!args.count("input_db") || !args.count("witness")) {
			cerr << "Error: At least 2 positional arguments (input_db and witness) are required." << endl;
			exit(1);
		}
		else {
			input_db_name = args["input_db"].as<string>();
			primary_wit_id = args["witness"].as<string>();
		}
		if (args.count("secondary_witnesses")) {
			vector<string> secondary_witnesses = args["secondary_witnesses"].as<vector<string>>();
			for (string secondary_wit_id : secondary_witnesses) {
				//The primary witness's ID should not occur again as a secondary witness:
				if (secondary_wit_id == primary_wit_id) {
					cerr << "Error: the primary witness ID should not be included in the list of secondary witnesses." << endl;
					exit(1);
				}
				//Add this entry to the ordered list if it hasn't already been added:
				if (secondary_wit_ids.find(secondary_wit_id) == secondary_wit_ids.end()) {
					secondary_wit_ids_ordered.push_back(secondary_wit_id);
				}
				secondary_wit_ids.insert(secondary_wit_id);
			}
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
	cout << "Retrieving genealogical relationships for witnesses..." << endl;
	//Retrieve a list of all witness IDs:
	list<string> list_wit = get_witness_ids(input_db);
	//Retrieve all necessary genealogical comparisons relative to the primary witness:
	unordered_map<string, genealogical_comparison> primary_witness_genealogical_comparisons = get_primary_witness_genealogical_comparisons(input_db, primary_wit_id);
	//Retrieve all necessary genealogical comparisons relative to the secondary witnesses:
	unordered_map<string, unordered_map<string, genealogical_comparison>> secondary_witness_genealogical_comparisons = get_secondary_witness_genealogical_comparisons(input_db, primary_wit_id);
	//Close the database:
	cout << "Closing database..." << endl;
	sqlite3_close(input_db);
	cout << "Database closed." << endl;
	//Now calculate the comparison metrics between the primary witness and all of the secondary witnesses:
	list<witness_comparison> comparisons = list<witness_comparison>();
	for (string secondary_wit_id : list_wit) {
		//Skip the primary witness:
		if (secondary_wit_id == primary_wit_id) {
			continue;
		}
		genealogical_comparison primary_primary_comp = primary_witness_genealogical_comparisons.at(primary_wit_id);
		genealogical_comparison primary_secondary_comp = primary_witness_genealogical_comparisons.at(secondary_wit_id);
		genealogical_comparison secondary_primary_comp = secondary_witness_genealogical_comparisons.at(secondary_wit_id).at(primary_wit_id);
		genealogical_comparison secondary_secondary_comp = secondary_witness_genealogical_comparisons.at(secondary_wit_id).at(secondary_wit_id);
		Roaring primary_extant = primary_primary_comp.explained;
		Roaring secondary_extant = secondary_secondary_comp.explained;
		Roaring mutually_extant = primary_extant & secondary_extant;
		Roaring agreements = primary_secondary_comp.agreements;
		Roaring primary_explained_by_secondary = primary_secondary_comp.explained;
		Roaring secondary_explained_by_primary = secondary_primary_comp.explained;
		witness_comparison comparison;
		comparison.id = secondary_wit_id;
		comparison.pass = mutually_extant.cardinality();
		comparison.eq = agreements.cardinality();
		comparison.prior = (secondary_explained_by_primary ^ agreements).cardinality();
		comparison.posterior = (primary_explained_by_secondary ^ agreements).cardinality();
		comparison.norel = comparison.pass - comparison.eq - comparison.prior - comparison.posterior;
		comparison.perc = comparison.pass > 0 ? (100 * float(comparison.eq) / float(comparison.pass)) : 0;
		comparison.dir = comparison.prior > comparison.posterior ? -1 : (comparison.posterior > comparison.prior ? 1 : 0);
		comparison.cost = comparison.dir < 0 ? -1 : primary_secondary_comp.cost;
		comparisons.push_back(comparison);
	}
	//Sort the list of comparisons from highest number of agreements to lowest:
	comparisons.sort([](const witness_comparison & wc1, const witness_comparison & wc2) {
		return wc1.eq > wc2.eq;
	});
	//Pass through the sorted list of comparisons to assign ancestral ranks:
	int nr = 0;
	int nr_value = numeric_limits<int>::max();
	for (witness_comparison & comparison : comparisons) {
		//Only assign positive ancestral ranks to witnesses prior to this one:
		if (comparison.dir == 1) {
			//Only increment the rank if the number of agreements is lower than that of the previous potential ancestor:
			if (comparison.eq < nr_value) {
				nr_value = comparison.eq;
				nr++;
			}
			comparison.nr = nr;

		}
		else if (comparison.dir == 0) {
			comparison.nr = 0;
		}
		else {
			comparison.nr = -1;
		}
	}
	print_comparisons(primary_wit_id, secondary_wit_ids, comparisons);
	exit(0);
}
