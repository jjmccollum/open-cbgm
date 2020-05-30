/*
 * find_relatives.cpp
 *
 *  Created on: Nov 26, 2019
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
	list<string> rdgs; //readings of the secondary witness at the given variation unit
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
 * Retrieves rows for the given variation unit from the READING_SUPPORT table of the given SQLite database
 * and returns a reading support map populated with its contents.
 */
unordered_map<string, list<string>> get_reading_support(sqlite3 * input_db, const string & vu_id) {
	unordered_map<string, list<string>> reading_support = unordered_map<string, list<string>>();
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_reading_support_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM READING_SUPPORT WHERE VARIATION_UNIT=?", -1, & select_from_reading_support_stmt, 0);
	sqlite3_bind_text(select_from_reading_support_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_reading_support_stmt);
	while (rc == SQLITE_ROW) {
		string wit_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_reading_support_stmt, 1)));
		string rdg_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_reading_support_stmt, 2)));
		//Add an entry for this witness if there isn't one already:
		if (reading_support.find(wit_id) == reading_support.end()) {
			reading_support[wit_id] = list<string>();
		}
		reading_support[wit_id].push_back(rdg_id);
		rc = sqlite3_step(select_from_reading_support_stmt);
	}
	sqlite3_finalize(select_from_reading_support_stmt);
	return reading_support;
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
 * Determines if the VARIATION_UNITS table of the given SQLite database
 * contains a row with the given variation unit ID.
 */
bool variation_unit_id_exists(sqlite3 * input_db, const string & vu_id) {
	bool variation_unit_id_exists = false;
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_variation_units_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM VARIATION_UNITS WHERE VARIATION_UNIT=?", -1, & select_from_variation_units_stmt, 0);
	sqlite3_bind_text(select_from_variation_units_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_variation_units_stmt);
	variation_unit_id_exists = (rc == SQLITE_ROW) ? true : false;
	sqlite3_finalize(select_from_variation_units_stmt);
	return variation_unit_id_exists;
}

/**
 * Retrieves all rows from the VARIATION_UNITS table of the given SQLite database
 * and returns a vector of variation_unit IDs populated with its contents.
 */
vector<string> get_variation_unit_ids(sqlite3 * input_db) {
	vector<string> variation_unit_ids = vector<string>();
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_variation_units_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM VARIATION_UNITS ORDER BY ROWID", -1, & select_from_variation_units_stmt, 0);
	rc = sqlite3_step(select_from_variation_units_stmt);
	while (rc == SQLITE_ROW) {
		string vu_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_variation_units_stmt, 0)));
		variation_unit_ids.push_back(vu_id);
		rc = sqlite3_step(select_from_variation_units_stmt);
	}
	sqlite3_finalize(select_from_variation_units_stmt);
	return variation_unit_ids;
}

/**
 * Returns the label for the given variation unit ID from the VARIATION_UNITS table of the given SQLite database.
 */
string get_variation_unit_label(sqlite3 * input_db, const string & vu_id) {
	string variation_unit_label = string();
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_variation_units_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM VARIATION_UNITS WHERE VARIATION_UNIT=?", -1, & select_from_variation_units_stmt, 0);
	sqlite3_bind_text(select_from_variation_units_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_variation_units_stmt);
	while (rc == SQLITE_ROW) {
		variation_unit_label = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_variation_units_stmt, 1)));
		break;
	}
	sqlite3_finalize(select_from_variation_units_stmt);
	return variation_unit_label;
}

/**
 * Given primary witness ID, a variation unit label, a list of readings supported by the primary witness,
 * a filter reading, and a list of witness comparisons (assumed to be sorted in decreasing order of agreements),
 * prints the relatives list for the primary witness at the given variation unit.
 */
void print_relatives(const string & primary_wit_id, const string & vu_label, const list<string> & primary_wit_readings, const string & filter_reading, const list<witness_comparison> & comparisons) {
	//Print the caption:
	if (filter_reading.empty()) {
		cout << "Relatives of W1 = " << primary_wit_id << " at " << vu_label << " ";
	}
	else {
		cout << "Relatives of W1 = " << primary_wit_id << " at " << vu_label << " with reading " << filter_reading << " ";
	}
	if (!primary_wit_readings.empty()) {
		cout << "(W1 RDG = ";
		for (string primary_wit_rdg : primary_wit_readings) {
			if (primary_wit_rdg != primary_wit_readings.front()) {
				cout << ", ";
			}
			cout << primary_wit_rdg;
		}
		cout << "):\n\n";
	}
	else {
		cout << "(W1 is lacunose):\n\n";
	}
	//Print the header row:
	cout << std::left << std::setw(8) << "W2";
	cout << std::left << std::setw(4) << "DIR";
	cout << std::right << std::setw(4) << "NR";
	cout << std::setw(4) << ""; //buffer space between right-aligned and left-aligned columns
	cout << std::left << std::setw(8) << "RDG";
	cout << std::right << std::setw(8) << "PASS";
	cout << std::right << std::setw(8) << "EQ";
	cout << std::right << std::setw(12) << ""; //percentage of agreements among mutually extant passages
	cout << std::right << std::setw(8) << "W1>W2";
	cout << std::right << std::setw(8) << "W1<W2";
	//cout << std::right << std::setw(8) << "UNCL";
	cout << std::right << std::setw(8) << "NOREL";
	cout << std::right << std::setw(12) << "COST";
	cout << "\n\n";
	//Print the subsequent rows:
	for (witness_comparison comparison : comparisons) {
		string rdgs_str = "";
		bool match = filter_reading.empty() ? true : false;
		for (string rdg : comparison.rdgs) {
			if (rdg != comparison.rdgs.front()) {
				rdgs_str += ", ";
			}
			rdgs_str += rdg;
			if (rdg == filter_reading) {
				match = true;
			}
		}
		if (!match) {
			continue;
		}
		//Handle lacunae:
		if (rdgs_str.empty()) {
			rdgs_str = "-";
		}
		cout << std::left << std::setw(8) << comparison.id;
		cout << std::left << std::setw(4) << (comparison.dir == -1 ? "<" : (comparison.dir == 1 ? ">" : "="));
		cout << std::right << std::setw(4) << (comparison.nr > 0 ? to_string(comparison.nr) : "");
		cout << std::setw(4) << ""; //buffer space between right-aligned and left-aligned columns
		cout << std::left << std::setw(8) << rdgs_str;
		cout << std::right << std::setw(8) << comparison.pass;
		cout << std::right << std::setw(8) << comparison.eq;
		cout << std::right << std::setw(3) << "(" << std::setw(7) << fixed << std::setprecision(3) << comparison.perc << std::setw(2) << "%)";
		cout << std::right << std::setw(8) << comparison.prior;
		cout << std::right << std::setw(8) << comparison.posterior;
		//cout << std::right << std::setw(8) << comparison.uncl;
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
	string filter_reading = string();
	string input_db_name = string();
	string primary_wit_id = string();
	string vu_id = string();
	try {
		cxxopts::Options options("find_relatives", "Get a table of genealogical relationships between the witness with the given ID and other witnesses at a given passage, as specified by the user.\nOptionally, the user can optionally specify a reading ID for the given passage, in which case the output will be restricted to the witnesses preserving that reading.");
		options.custom_help("[-h] [-r reading] input_db witness passage");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("r,reading", "ID of desired variant reading", cxxopts::value<string>());
		options.add_options("positional")
				("input_db", "genealogical cache database", cxxopts::value<string>())
				("witness", "ID of the witness whose relatives are desired, as found in its <witness> element in the XML file", cxxopts::value<string>())
				("passage", "ID or (one-based) index of the variation unit at which relatives' readings are desired", cxxopts::value<vector<string>>());
		options.parse_positional({"input_db", "witness", "passage"});
		auto args = options.parse(argc, argv);
		//Print help documentation and exit if specified:
		if (args.count("help")) {
			cout << options.help({""}) << endl;
			exit(0);
		}
		//Parse the optional arguments:
		if (args.count("r")) {
			filter_reading = args["r"].as<string>();
		}
		//Parse the positional arguments:
		if (!args.count("input_db") || !args.count("witness") || args.count("passage") != 1) {
			cerr << "Error: 3 positional arguments (input_db, witness, and passage) are required." << endl;
			exit(1);
		}
		else {
			input_db_name = args["input_db"].as<string>();
			primary_wit_id = args["witness"].as<string>();
			vu_id = args["passage"].as<vector<string>>()[0];
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
	//Retrieve a map of reading support for this variation unit:
	unordered_map<string, list<string>> reading_support = get_reading_support(input_db, vu_id);
	//Retrieve all necessary genealogical comparisons relative to the primary witness:
	unordered_map<string, genealogical_comparison> primary_witness_genealogical_comparisons = get_primary_witness_genealogical_comparisons(input_db, primary_wit_id);
	//Retrieve all necessary genealogical comparisons relative to the secondary witnesses:
	unordered_map<string, unordered_map<string, genealogical_comparison>> secondary_witness_genealogical_comparisons = get_secondary_witness_genealogical_comparisons(input_db, primary_wit_id);
	cout << "Retrieving variation units..." << endl;
	//Retrieve a vector of all variation unit IDs:
	vector<string> vu_ids = get_variation_unit_ids(input_db);
	//Check if the input passage is a variation unit ID in the database:
	bool variation_unit_matched = variation_unit_id_exists(input_db, vu_id);
	//If no match is found, the try to treat the ID as an index:
	if (!variation_unit_matched) {
		bool is_number = true;
		for (string::const_iterator it = vu_id.begin(); it != vu_id.end(); it++) {
			if (!isdigit(*it)) {
				is_number = false;
				break;
			}
		}
		//If it isn't a number, then report an error to the user and exit:
		if (!is_number) {
			cerr << "Error: The VARIATION_UNITS table has no rows with VARIATION_UNIT = " << vu_id << "." << endl;
			exit(1);
		}
		//Otherwise, convert the ID to a zero-based index, and check if it is a valid index:
		unsigned int vu_ind = atoi(vu_id.c_str()) - 1;
		if (vu_ind >= vu_ids.size()) {
			cerr << "Error: The VARIATION_UNITS table has no rows with VARIATION_UNIT = " << vu_id << "; if the variation unit ID was specified as an index, then it is out of range, as there are only " << vu_ids.size() << " variation units." << endl;
			exit(1);
		}
		//Otherwise, get the variation unit corresponding to this ID:
		vu_id = vu_ids[vu_ind];
	}
	//Get the variation unit's label:
	string vu_label = get_variation_unit_label(input_db, vu_id);
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
		comparison.rdgs = list<string>();
		if (reading_support.find(secondary_wit_id) != reading_support.end()) {
			comparison.rdgs = reading_support[secondary_wit_id];
		}
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
	list<string> primary_wit_readings = reading_support.find(primary_wit_id) != reading_support.end() ? reading_support.at(primary_wit_id) : list<string>();
	print_relatives(primary_wit_id, vu_label, primary_wit_readings, filter_reading, comparisons);
	exit(0);
}
