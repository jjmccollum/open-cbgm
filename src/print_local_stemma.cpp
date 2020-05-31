/*
 * print_local_stemma.cpp
 *
 *  Created on: Jan 13, 2020
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
#include "local_stemma.h"

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
 * Using rows from the READINGS and READING_RELATIONS tables of the given SQLite database with the given variation unit ID,
 * constructs and returns the corresponding local stemma.
 */
local_stemma get_local_stemma_for_variation_unit(sqlite3 * input_db, const string & vu_id) {
	local_stemma stemma = local_stemma();
	int rc; //to store SQLite macros
	string id = vu_id;
	//Get the variation unit label first:
	string label = string();
	sqlite3_stmt * select_from_variation_units_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM VARIATION_UNITS WHERE VARIATION_UNITS.VARIATION_UNIT=?", -1, & select_from_variation_units_stmt, 0);
	sqlite3_bind_text(select_from_variation_units_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_variation_units_stmt);
	while (rc == SQLITE_ROW) {
		label = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_variation_units_stmt, 1)));
		break;
	}
	sqlite3_finalize(select_from_variation_units_stmt);
	//Then populate the local stemma graph:
	local_stemma_graph graph;
	//Add the vertices of the local stemma:
	graph.vertices = list<local_stemma_vertex>();
	sqlite3_stmt * select_from_readings_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM READINGS WHERE READINGS.VARIATION_UNIT=?", -1, & select_from_readings_stmt, 0);
	sqlite3_bind_text(select_from_readings_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_readings_stmt);
	while (rc == SQLITE_ROW) {
		local_stemma_vertex v;
		v.id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_readings_stmt, 1)));
		graph.vertices.push_back(v);
		rc = sqlite3_step(select_from_readings_stmt);
	}
	sqlite3_finalize(select_from_readings_stmt);
	//Add the edges of the local_stemma:
	graph.edges = list<local_stemma_edge>();
	sqlite3_stmt * select_from_reading_relations_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM READING_RELATIONS WHERE READING_RELATIONS.VARIATION_UNIT=?", -1, & select_from_reading_relations_stmt, 0);
	sqlite3_bind_text(select_from_reading_relations_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_reading_relations_stmt);
	while (rc == SQLITE_ROW) {
		local_stemma_edge e;
		e.prior = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_reading_relations_stmt, 1)));
		e.posterior = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_reading_relations_stmt, 2)));
		e.weight = float(sqlite3_column_double(select_from_reading_relations_stmt, 3));
		graph.edges.push_back(e);
		rc = sqlite3_step(select_from_reading_relations_stmt);
	}
	sqlite3_finalize(select_from_reading_relations_stmt);
	stemma = local_stemma(id, label, graph);
	return stemma;
}

/**
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Read in the command-line options:
	bool print_weights = false;
	set<string> filter_vu_ids = set<string>();
	string input_db_name = string();
	try {
		cxxopts::Options options("print_local_stemma", "Print local stemma graphs to .dot output files. The output files will be placed in the \"local\" directory.");
		options.custom_help("[-h] [--weights] input_db [passages]");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("weights", "print edge weights", cxxopts::value<bool>());
		options.add_options("positional")
				("input_db", "genealogical cache database", cxxopts::value<string>())
				("passages", "if specified, only print graphs for the variation units with the given IDs or (one-based) indices; otherwise, print graphs for all variation units", cxxopts::value<vector<string>>());
		options.parse_positional({"input_db", "passages"});
		auto args = options.parse(argc, argv);
		//Print help documentation and exit if specified:
		if (args.count("help")) {
			cout << options.help({""}) << endl;
			exit(0);
		}
		//Parse the optional arguments:
		if (args.count("weights")) {
			print_weights = args["weights"].as<bool>();
		}
		//Parse the positional arguments:
		if (!args.count("input_db")) {
			cerr << "Error: 1 positional argument (input_db) is required." << endl;
			exit(1);
		}
		else {
			input_db_name = args["input_db"].as<string>();
		}
		if (args.count("passages")) {
			vector<string> passages = args["passages"].as<vector<string>>();
			for (string filter_vu_id : passages) {
				filter_vu_ids.insert(filter_vu_id);
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
	cout << "Retrieving local stemmata for variation units..." << endl;
	//Retrieve a vector of all variation unit IDs:
	vector<string> vu_ids = get_variation_unit_ids(input_db);
	//Then populate a list of IDs for all variation units to process:
	list<string> vu_ids_to_process = list<string>();
	//If no filter variation unit IDs were specified, then process all variation units:
	if (filter_vu_ids.empty()) {
		for (string vu_id : vu_ids) {
			vu_ids_to_process.push_back(vu_id);
		}
	}
	//If any filter variation unit IDs were specified, then make sure they are valid:
	else {
		for (string filter_vu_id : filter_vu_ids) {
			//Check if the input passage is a variation unit ID in the database:
			bool variation_unit_matched = variation_unit_id_exists(input_db, filter_vu_id);
			//If a match is found, then add the variation unit ID to the list to be processed:
			if (variation_unit_matched) {
				vu_ids_to_process.push_back(filter_vu_id);
			}
			//Otherwise, try to treat the ID as an index:
			else {
				bool is_number = true;
				for (string::const_iterator it = filter_vu_id.begin(); it != filter_vu_id.end(); it++) {
					if (!isdigit(*it)) {
						is_number = false;
						break;
					}
				}
				//If it isn't a number, then report an error to the user and exit:
				if (!is_number) {
					cerr << "Error: The VARIATION_UNITS table has no rows with VARIATION_UNIT = " << filter_vu_id << "." << endl;
					exit(1);
				}
				//Otherwise, convert the ID to a zero-based index, and check if it is a valid index:
				unsigned int filter_vu_ind = atoi(filter_vu_id.c_str()) - 1;
				if (filter_vu_ind >= vu_ids.size()) {
					cerr << "Error: The VARIATION_UNITS table has no rows with VARIATION_UNIT = " << filter_vu_id << "; if the variation unit ID was specified as an index, then it is out of range, as there are only " << vu_ids.size() << " variation units." << endl;
					exit(1);
				}
				//Otherwise, get the variation unit corresponding to this ID:
				vu_ids_to_process.push_back(vu_ids[filter_vu_ind]);
			}
		}
	}
	list<local_stemma> local_stemmata = list<local_stemma>();
	//Now populate a list of local stemmata:
	for (string vu_id : vu_ids_to_process) {
		//Get the local stemma for this variation unit:
		local_stemma ls = get_local_stemma_for_variation_unit(input_db, vu_id);
		local_stemmata.push_back(ls);
	}
	//Close the database:
	cout << "Closing database..." << endl;
	sqlite3_close(input_db);
	cout << "Database closed." << endl;
	cout << "Generating local stemmata..." << endl;
	//Create the directory to write files to:
	string local_dir = "local";
	create_dir(local_dir);
	//Now generate the graph for each local stemma:
	for (local_stemma ls : local_stemmata) {
		string vu_id = ls.get_id();
		//Complete the path to this file:
		string filepath = local_dir + "/" + vu_id + "-local-stemma.dot";
		//Open a filestream:
		fstream dot_file;
		dot_file.open(filepath, ios::out);
		ls.to_dot(dot_file, print_weights);
		dot_file.close();
	}
	exit(0);
}
