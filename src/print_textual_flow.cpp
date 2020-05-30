/*
 * print_textual_flow.cpp
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
#include <limits>

#include "cxxopts.h"
#include "sqlite3.h"
#include "local_stemma.h"
#include "variation_unit.h"
#include "witness.h"
#include "textual_flow.h"


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
 * Retrieves rows for the given variation unit from the READINGS table of the given SQLite database
 * and returns a list of reading IDs populated with its contents.
 */
list<string> get_readings_for_variation_unit(sqlite3 * input_db, const string & vu_id) {
	list<string> readings = list<string>();
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_readings_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM READINGS WHERE READINGS.VARIATION_UNIT=? ORDER BY ROWID", -1, & select_from_readings_stmt, 0);
	sqlite3_bind_text(select_from_readings_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_readings_stmt);
	while (rc == SQLITE_ROW) {
		string rdg = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_readings_stmt, 1)));
		readings.push_back(rdg);
		rc = sqlite3_step(select_from_readings_stmt);
	}
	sqlite3_finalize(select_from_readings_stmt);
	return readings;
}

/**
 * Retrieves rows for the given variation unit from the READING_SUPPORT table of the given SQLite database
 * and returns a reading support map populated with its contents.
 */
unordered_map<string, string> get_reading_support_for_variation_unit(sqlite3 * input_db, const string & vu_id) {
	unordered_map<string, string> reading_support = unordered_map<string, string>();
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_reading_support_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM READING_SUPPORT WHERE VARIATION_UNIT=?", -1, & select_from_reading_support_stmt, 0);
	sqlite3_bind_text(select_from_reading_support_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_reading_support_stmt);
	while (rc == SQLITE_ROW) {
		string wit_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_reading_support_stmt, 1)));
		string rdg_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_reading_support_stmt, 2)));
		reading_support[wit_id] = rdg_id;
		rc = sqlite3_step(select_from_reading_support_stmt);
	}
	sqlite3_finalize(select_from_reading_support_stmt);
	return reading_support;
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
	sqlite3_prepare(input_db, "SELECT * FROM READINGS WHERE READINGS.VARIATION_UNIT=? ORDER BY ROWID", -1, & select_from_readings_stmt, 0);
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
	sqlite3_prepare(input_db, "SELECT * FROM READING_RELATIONS WHERE READING_RELATIONS.VARIATION_UNIT=? ORDER BY ROWID", -1, & select_from_reading_relations_stmt, 0);
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
 * Using rows from the VARIATION_UNITS table of the given SQLite database with the given variation unit ID,
 * constructs and returns the corresponding variation unit.
 */
variation_unit get_variation_unit(sqlite3 * input_db, const string & vu_id) {
	variation_unit vu = variation_unit();
	int rc; //to store SQLite macros
	string id = vu_id;
	//Get the variation unit label and connectivity first:
	string label = string();
	int connectivity = numeric_limits<int>::max();
	sqlite3_stmt * select_from_variation_units_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM VARIATION_UNITS WHERE VARIATION_UNITS.VARIATION_UNIT=?", -1, & select_from_variation_units_stmt, 0);
	sqlite3_bind_text(select_from_variation_units_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_variation_units_stmt);
	while (rc == SQLITE_ROW) {
		label = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_variation_units_stmt, 1)));
		connectivity = sqlite3_column_int(select_from_variation_units_stmt, 2);
		break;
	}
	sqlite3_finalize(select_from_variation_units_stmt);
	//Then get the readings list:
	list<string> readings = get_readings_for_variation_unit(input_db, vu_id);
	//Then get the reading support map:
	unordered_map<string, string> reading_support = get_reading_support_for_variation_unit(input_db, vu_id);
	//Then get the local stemma:
	local_stemma stemma = get_local_stemma_for_variation_unit(input_db, vu_id);
	//Then construct the variation unit:
	vu = variation_unit(id, label, readings, reading_support, connectivity, stemma);
	return vu;
}

/**
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Read in the command-line options:
	bool flow = false;
	bool attestations = false;
	bool variants = false;
	bool flow_strengths = false;
	set<string> filter_vu_ids = set<string>();
	string input_db_name = string();
	try {
		cxxopts::Options options("print_textual_flow", "Prints multiple types of textual flow diagrams to .dot output files. The output files will be placed in the \"flow\", \"attestations\", and \"variants\" directories.");
		options.custom_help("[-h] [--flow] [--attestations] [--variants] [--strengths] input_db [passages]");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("flow", "print complete textual flow diagrams", cxxopts::value<bool>())
				("attestations", "print coherence in attestation textual flow diagrams for all readings at all passages", cxxopts::value<bool>())
				("variants", "print coherence at variant passages diagrams (i.e., textual flow diagrams restricted to flow between different readings) at all passages", cxxopts::value<bool>())
				("strengths", "format edges to reflect flow strengths", cxxopts::value<bool>());
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
		if (args.count("flow")) {
			flow = args["flow"].as<bool>();
		}
		if (args.count("attestations")) {
			attestations = args["attestations"].as<bool>();
		}
		if (args.count("variants")) {
			variants = args["variants"].as<bool>();
		}
		//If no specific graph types are specified, then print all of them:
		if (!(flow || attestations || variants)) {
			flow = true;
			attestations = true;
			variants = true;
		}
		//Parse the optional arguments:
		if (args.count("strengths")) {
			flow_strengths = args["strengths"].as<bool>();
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
	//Then populate each witness's list of potential ancestors:
	for (witness & wit : witnesses) {
		wit.set_potential_ancestor_ids(witnesses);
	}
	cout << "Retrieving variation units..." << endl;
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
	//Now populate a list of variation units:
	list<variation_unit> variation_units = list<variation_unit>();
	for (string vu_id : vu_ids_to_process) {
		variation_unit vu = get_variation_unit(input_db, vu_id);
		variation_units.push_back(vu);
	}
	//Close the database:
	cout << "Closing database..." << endl;
	sqlite3_close(input_db);
	cout << "Database closed." << endl;
	cout << "Generating textual flow diagrams..." << endl;
	//Create the directories to write files to:
	string flow_dir = "flow";
	string attestations_dir = "attestations";
	string variants_dir = "variants";
	create_dir(flow_dir);
	create_dir(attestations_dir);
	create_dir(variants_dir);
	//Now generate the graphs for each variation unit:
	for (variation_unit vu : variation_units) {
		string vu_id = vu.get_id();
		//Construct the underlying textual flow data structure using this variation unit and the list of witnesses:
		textual_flow tf = textual_flow(vu, witnesses);
		if (flow) {
			//Complete the path to the file:
			string filepath = flow_dir + "/" + vu_id + "-textual-flow.dot";
			//Then write to file:
			fstream dot_file;
			dot_file.open(filepath, ios::out);
			tf.textual_flow_to_dot(dot_file, flow_strengths);
			dot_file.close();
		}
		if (attestations) {
			//A separate coherence in attestations diagram is drawn for each reading:
			for (string rdg : vu.get_readings()) {
				//Complete the path to the file:
				string filepath = attestations_dir + "/" + vu_id + "R" + rdg + "-coherence-attestations.dot";
				//Then write to file:
				fstream dot_file;
				dot_file.open(filepath, ios::out);
				tf.coherence_in_attestations_to_dot(dot_file, rdg, flow_strengths);
				dot_file.close();
			}
		}
		if (variants) {
			//Complete the path to the file:
			string filepath = variants_dir + "/" + vu_id + "-coherence-variants.dot";
			//Then write to file:
			fstream dot_file;
			dot_file.open(filepath, ios::out);
			tf.coherence_in_variant_passages_to_dot(dot_file, flow_strengths);
			dot_file.close();
		}
	}
	exit(0);
}
