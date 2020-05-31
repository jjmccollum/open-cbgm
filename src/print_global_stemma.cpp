/*
 * print_global_stemma.cpp
 *
 *  Created on: Jan 14, 2020
 *      Author: jjmccollum
 */

#ifdef _WIN32
	#include <direct.h> //for Windows _mkdir() support
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <unordered_map>

#include "cxxopts.h"
#include "sqlite3.h"
#include "witness.h"
#include "global_stemma.h"


using namespace std;

/**
 * Creates a directory with the given name.
 * The return value will be 0 if successful, and 1 otherwise.
 */
int create_dir(const string & dir) {
	#ifdef _WIN32
		return _mkdir(dir.c_str());
	#else
		umask(0); //this is done to ensure that the newly created directory will have exactly the permissions we specify below
		return mkdir(dir.c_str(), 0755);
	#endif
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
 * Retrieves rows relative to the given witness ID from the GENEALOGICAL_COMPARISONS table of the given SQLite database
 * and returns a map of genealogical comparisons populated with its contents.
 */
unordered_map<string, genealogical_comparison> get_genealogical_comparisons_for_witness(sqlite3 * input_db, const string & wit_id) {
	unordered_map<string, genealogical_comparison> genealogical_comparisons = unordered_map<string, genealogical_comparison>();
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_genealogical_comparisons_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM GENEALOGICAL_COMPARISONS WHERE PRIMARY_WIT=?", -1, & select_from_genealogical_comparisons_stmt, 0);
	sqlite3_bind_text(select_from_genealogical_comparisons_stmt, 1, wit_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_genealogical_comparisons_stmt);
	while (rc == SQLITE_ROW) {
		genealogical_comparison comp;
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
		genealogical_comparisons[secondary_wit_id] = comp;
		rc = sqlite3_step(select_from_genealogical_comparisons_stmt);
	}
	sqlite3_finalize(select_from_genealogical_comparisons_stmt);
	return genealogical_comparisons;
}

/**
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Read in the command-line options:
	bool print_lengths = false;
	bool flow_strengths = false;
	string input_db_name = string();
	try {
		cxxopts::Options options("print_global_stemma", "Print a global stemma graph to a .dot output files. The output file will be placed in the \"global\" directory.");
		options.custom_help("[-h] [--lengths] [--strengths] input_db");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("lengths", "print genealogical costs as edge lengths")
				("strengths", "format edges to reflect flow strengths");
		options.add_options("positional")
				("input_db", "genealogical cache database", cxxopts::value<vector<string>>());
		options.parse_positional({"input_db"});
		auto args = options.parse(argc, argv);
		//Print help documentation and exit if specified:
		if (args.count("help")) {
			cout << options.help({""}) << endl;
			exit(0);
		}
		if (args.count("lengths")) {
			print_lengths = args["lengths"].as<bool>();
		}
		if (args.count("strengths")) {
			flow_strengths = args["strengths"].as<bool>();
		}
		//Parse the positional arguments:
		if (!args.count("input_db")) {
			cerr << "Error: 1 positional argument (input_db) is required." << endl;
			exit(1);
		}
		else {
			input_db_name = args["input_db"].as<vector<string>>()[0];
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
	cout << "Retrieving genealogical relationships for all witnesses..." << endl;
	//Populate a list of witness IDs:
	list<string> list_wit = get_witness_ids(input_db);
	//Populate a list of witnesses:
	list<witness> witnesses = list<witness>();
	for (string wit_id : list_wit) {
		unordered_map<string, genealogical_comparison> genealogical_comparisons = get_genealogical_comparisons_for_witness(input_db, wit_id);
		witness wit = witness(wit_id, genealogical_comparisons);
		witnesses.push_back(wit);
	}
	//Close the database:
	cout << "Closing database..." << endl;
	sqlite3_close(input_db);
	cout << "Database closed." << endl;
	cout << "Optimizing substemmata (this may take a moment)..." << endl;
	//Then populate each witness's list of potential ancestors and optimize its substemma:
	for (witness & wit : witnesses) {
		wit.set_potential_ancestor_ids(witnesses);
		wit.set_global_stemma_ancestor_ids();
	}
	cout << "Generating global stemma..." << endl;
	//Construct the global stemma using the witnesses:
	global_stemma gs = global_stemma(witnesses);
	//Create the directory to write the file to:
	string global_dir = "global";
	create_dir(global_dir);
	//Complete the path to the file:
	string filepath = global_dir + "/" + "global-stemma.dot";
	//Then write to file:
	fstream dot_file;
	dot_file.open(filepath, ios::out);
	gs.to_dot(dot_file, print_lengths, flow_strengths);
	dot_file.close();
	exit(0);
}
