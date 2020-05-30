/*
 * populate_db.cpp
 *
 *  Created on: Jan 10, 2020
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <unordered_map>

#include "cxxopts.h"
#include "pugixml.h"
#include "roaring.hh"
#include "witness.h"
#include "apparatus.h"
#include "variation_unit.h"
#include "local_stemma.h"
#include "sqlite3.h"

using namespace std;

/**
 * Creates, indexes, and populates the READINGS table.
 */
void populate_readings_table(sqlite3 * output_db, const list<variation_unit> & variation_units) {
	int rc; //to store SQLite macros
	cout << "Populating table READINGS..." << endl;
	//Create the READINGS table:
	string create_readings_sql = "DROP TABLE IF EXISTS READINGS;"
			"CREATE TABLE READINGS ("
			"VARIATION_UNIT TEXT NOT NULL, "
			"READING TEXT NOT NULL);";
	char * create_readings_error_msg;
	rc = sqlite3_exec(output_db, create_readings_sql.c_str(), NULL, 0, & create_readings_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating table READINGS: " << create_readings_error_msg << endl;
		sqlite3_free(create_readings_error_msg);
		exit(1);
	}
	//Denormalize it:
	string create_readings_idx_sql = "DROP INDEX IF EXISTS READINGS_IDX;"
			"CREATE INDEX READINGS_IDX ON READINGS (VARIATION_UNIT, READING);";
	char * create_readings_idx_error_msg;
	rc = sqlite3_exec(output_db, create_readings_idx_sql.c_str(), NULL, 0, & create_readings_idx_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating index READINGS_IDX: " << create_readings_idx_error_msg << endl;
		sqlite3_free(create_readings_idx_error_msg);
		exit(1);
	}
	//Then populate it using prepared statements within a single transaction:
	char * transaction_error_msg;
	sqlite3_exec(output_db, "BEGIN TRANSACTION", NULL, NULL, & transaction_error_msg);
	sqlite3_stmt * insert_into_readings_stmt;
	rc = sqlite3_prepare(output_db, "INSERT INTO READINGS VALUES (?,?)", -1, & insert_into_readings_stmt, 0);
	if (rc != SQLITE_OK) {
		cerr << "Error preparing statement." << endl;
		exit(1);
	}
	for (variation_unit vu : variation_units) {
		string vu_id = vu.get_id();
		local_stemma ls = vu.get_local_stemma();
		list<local_stemma_vertex> vertices = ls.get_graph().vertices;
		for (local_stemma_vertex v : vertices) {
			//Then insert a row containing these values:
			sqlite3_bind_text(insert_into_readings_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(insert_into_readings_stmt, 2, v.id.c_str(), -1, SQLITE_STATIC);
			rc = sqlite3_step(insert_into_readings_stmt);
			if (rc != SQLITE_DONE) {
				cerr << "Error executing prepared statement." << endl;
				exit(1);
			}
			//Then reset the prepared statement so we can bind the next values to it:
			sqlite3_reset(insert_into_readings_stmt);
		}
	}
	sqlite3_finalize(insert_into_readings_stmt);
	sqlite3_exec(output_db, "END TRANSACTION", NULL, NULL, & transaction_error_msg);
	return;
}

/**
 * Creates, indexes, and populates the READING_RELATIONS table.
 */
void populate_reading_relations_table(sqlite3 * output_db, const list<variation_unit> & variation_units) {
	int rc; //to store SQLite macros
	cout << "Populating table READING_RELATIONS..." << endl;
	//Create the READING_RELATIONS table:
	string create_reading_relations_sql = "DROP TABLE IF EXISTS READING_RELATIONS;"
			"CREATE TABLE READING_RELATIONS ("
			"VARIATION_UNIT TEXT NOT NULL, "
			"PRIOR TEXT NOT NULL, "
			"POSTERIOR TEXT NOT NULL, "
			"WEIGHT REAL NOT NULL);";
	char * create_reading_relations_error_msg;
	rc = sqlite3_exec(output_db, create_reading_relations_sql.c_str(), NULL, 0, & create_reading_relations_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating table READING_RELATIONS: " << create_reading_relations_error_msg << endl;
		sqlite3_free(create_reading_relations_error_msg);
		exit(1);
	}
	//Denormalize it:
	string create_reading_relations_idx_sql = "DROP INDEX IF EXISTS READING_RELATIONS_IDX;"
			"CREATE INDEX READING_RELATIONS_IDX ON READING_RELATIONS (VARIATION_UNIT, PRIOR, POSTERIOR);";
	char * create_reading_relations_idx_error_msg;
	rc = sqlite3_exec(output_db, create_reading_relations_idx_sql.c_str(), NULL, 0, & create_reading_relations_idx_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating index READING_RELATIONS_IDX: " << create_reading_relations_idx_error_msg << endl;
		sqlite3_free(create_reading_relations_idx_error_msg);
		exit(1);
	}
	//Then populate it using prepared statements within a single transaction:
	char * transaction_error_msg;
	sqlite3_exec(output_db, "BEGIN TRANSACTION", NULL, NULL, & transaction_error_msg);
	sqlite3_stmt * insert_into_reading_relations_stmt;
	rc = sqlite3_prepare(output_db, "INSERT INTO READING_RELATIONS VALUES (?,?,?,?)", -1, & insert_into_reading_relations_stmt, 0);
	if (rc != SQLITE_OK) {
		cerr << "Error preparing statement." << endl;
		exit(1);
	}
	for (variation_unit vu : variation_units) {
		string vu_id = vu.get_id();
		local_stemma ls = vu.get_local_stemma();
		list<local_stemma_edge> edges = ls.get_graph().edges;
		for (local_stemma_edge e : edges) {
			//Then insert a row containing these values:
			sqlite3_bind_text(insert_into_reading_relations_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(insert_into_reading_relations_stmt, 2, e.prior.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(insert_into_reading_relations_stmt, 3, e.posterior.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_double(insert_into_reading_relations_stmt, 4, e.weight);
			rc = sqlite3_step(insert_into_reading_relations_stmt);
			if (rc != SQLITE_DONE) {
				cerr << "Error executing prepared statement." << endl;
				exit(1);
			}
			//Then reset the prepared statement so we can bind the next values to it:
			sqlite3_reset(insert_into_reading_relations_stmt);
		}
	}
	sqlite3_finalize(insert_into_reading_relations_stmt);
	sqlite3_exec(output_db, "END TRANSACTION", NULL, NULL, & transaction_error_msg);
	return;
}

/**
 * Creates, indexes, and populates the READING_SUPPORT table.
 */
void populate_reading_support_table(sqlite3 * output_db, const list<variation_unit> & variation_units) {
	int rc; //to store SQLite macros
	cout << "Populating table READING_SUPPORT..." << endl;
	//Create the READING_SUPPORT table:
	string create_reading_support_sql = "DROP TABLE IF EXISTS READING_SUPPORT;"
			"CREATE TABLE READING_SUPPORT ("
			"VARIATION_UNIT TEXT NOT NULL, "
			"WITNESS TEXT NOT NULL, "
			"READING TEXT NOT NULL);";
	char * create_reading_support_error_msg;
	rc = sqlite3_exec(output_db, create_reading_support_sql.c_str(), NULL, 0, & create_reading_support_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating table READING_SUPPORT: " << create_reading_support_error_msg << endl;
		sqlite3_free(create_reading_support_error_msg);
		exit(1);
	}
	//Denormalize it:
	string create_reading_support_idx_sql = "DROP INDEX IF EXISTS READING_SUPPORT_IDX;"
			"CREATE INDEX READING_SUPPORT_IDX ON READING_SUPPORT (VARIATION_UNIT, WITNESS, READING);";
	char * create_reading_support_idx_error_msg;
	rc = sqlite3_exec(output_db, create_reading_support_idx_sql.c_str(), NULL, 0, & create_reading_support_idx_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating index READING_SUPPORT_IDX: " << create_reading_support_idx_error_msg << endl;
		sqlite3_free(create_reading_support_idx_error_msg);
		exit(1);
	}
	//Then populate it using prepared statements within a single transaction:
	char * transaction_error_msg;
	sqlite3_exec(output_db, "BEGIN TRANSACTION", NULL, NULL, & transaction_error_msg);
	sqlite3_stmt * insert_into_reading_support_stmt;
	rc = sqlite3_prepare(output_db, "INSERT INTO READING_SUPPORT VALUES (?,?,?)", -1, & insert_into_reading_support_stmt, 0);
	if (rc != SQLITE_OK) {
		cerr << "Error preparing statement." << endl;
		exit(1);
	}
	for (variation_unit vu : variation_units) {
		string vu_id = vu.get_id();
		unordered_map<string, string> reading_support = vu.get_reading_support();
		for (pair<string, string> kv : reading_support) {
			string wit_id = kv.first;
			string wit_rdg = kv.second;
			//Then insert a row containing these values:
			sqlite3_bind_text(insert_into_reading_support_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(insert_into_reading_support_stmt, 2, wit_id.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(insert_into_reading_support_stmt, 3, wit_rdg.c_str(), -1, SQLITE_STATIC);
			rc = sqlite3_step(insert_into_reading_support_stmt);
			if (rc != SQLITE_DONE) {
				cerr << "Error executing prepared statement." << endl;
				exit(1);
			}
			//Then reset the prepared statement so we can bind the next values to it:
			sqlite3_reset(insert_into_reading_support_stmt);
		}
	}
	sqlite3_finalize(insert_into_reading_support_stmt);
	sqlite3_exec(output_db, "END TRANSACTION", NULL, NULL, & transaction_error_msg);
	return;
}

/**
 * Creates, indexes, and populates the VARIATION_UNITS table.
 */
void populate_variation_units_table(sqlite3 * output_db, const list<variation_unit> & variation_units) {
	int rc; //to store SQLite macros
	cout << "Populating table VARIATION_UNITS..." << endl;
	//Create the VARIATION_UNITS table:
	string create_variation_units_sql = "DROP TABLE IF EXISTS VARIATION_UNITS;"
			"CREATE TABLE VARIATION_UNITS ("
			"VARIATION_UNIT TEXT NOT NULL, "
			"LABEL TEXT, "
			"CONNECTIVITY INT NOT NULL);";
	char * create_variation_units_error_msg;
	rc = sqlite3_exec(output_db, create_variation_units_sql.c_str(), NULL, 0, & create_variation_units_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating table VARIATION_UNITS: " << create_variation_units_error_msg << endl;
		sqlite3_free(create_variation_units_error_msg);
		exit(1);
	}
	//Denormalize it:
	string create_variation_units_idx_sql = "DROP INDEX IF EXISTS VARIATION_UNITS_IDX;"
			"CREATE INDEX VARIATION_UNITS_IDX ON VARIATION_UNITS (VARIATION_UNIT);";
	char * create_variation_units_idx_error_msg;
	rc = sqlite3_exec(output_db, create_variation_units_idx_sql.c_str(), NULL, 0, & create_variation_units_idx_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating index VARIATION_UNITS_IDX: " << create_variation_units_idx_error_msg << endl;
		sqlite3_free(create_variation_units_idx_error_msg);
		exit(1);
	}
	//Then populate it using prepared statements within a single transaction:
	char * transaction_error_msg;
	sqlite3_exec(output_db, "BEGIN TRANSACTION", NULL, NULL, & transaction_error_msg);
	sqlite3_stmt * insert_into_variation_units_stmt;
	rc = sqlite3_prepare(output_db, "INSERT INTO VARIATION_UNITS VALUES (?,?,?)", -1, & insert_into_variation_units_stmt, 0);
	if (rc != SQLITE_OK) {
		cerr << "Error preparing statement." << endl;
		exit(1);
	}
	for (variation_unit vu : variation_units) {
		string id = vu.get_id();
		string label = vu.get_label();
		int connectivity = vu.get_connectivity();
		//Then insert a row containing these values:
		sqlite3_bind_text(insert_into_variation_units_stmt, 1, id.c_str(), -1, SQLITE_STATIC);
		sqlite3_bind_text(insert_into_variation_units_stmt, 2, label.c_str(), -1, SQLITE_STATIC);
		sqlite3_bind_int(insert_into_variation_units_stmt, 3, connectivity);
		rc = sqlite3_step(insert_into_variation_units_stmt);
		if (rc != SQLITE_DONE) {
			cerr << "Error executing prepared statement." << endl;
			exit(1);
		}
		//Then reset the prepared statement so we can bind the next values to it:
		sqlite3_reset(insert_into_variation_units_stmt);
	}
	sqlite3_finalize(insert_into_variation_units_stmt);
	sqlite3_exec(output_db, "END TRANSACTION", NULL, NULL, & transaction_error_msg);
	return;
}

/**
 * Creates, indexes, and populates the GENEALOGICAL_COMPARISONS table.
 */
void populate_genealogical_comparisons_table(sqlite3 * output_db, const list<witness> & witnesses) {
	int rc; //to store SQLite macros
	cout << "Populating table GENEALOGICAL_COMPARISONS..." << endl;
	//Create the GENEALOGICAL_COMPARISONS table:
	string create_genealogical_comparisons_sql = "DROP TABLE IF EXISTS GENEALOGICAL_COMPARISONS;"
			"CREATE TABLE GENEALOGICAL_COMPARISONS ("
			"PRIMARY_WIT TEXT NOT NULL, "
			"SECONDARY_WIT TEXT NOT NULL, "
			"AGREEMENTS BLOB NOT NULL, "
			"EXPLAINED BLOB NOT NULL, "
			"COST REAL NOT NULL);";
	char * create_genealogical_comparisons_error_msg;
	rc = sqlite3_exec(output_db, create_genealogical_comparisons_sql.c_str(), NULL, 0, & create_genealogical_comparisons_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating table GENEALOGICAL_COMPARISONS: " << create_genealogical_comparisons_error_msg << endl;
		sqlite3_free(create_genealogical_comparisons_error_msg);
		exit(1);
	}
	//Denormalize it:
	string create_genealogical_comparisons_idx_sql = "DROP INDEX IF EXISTS GENEALOGICAL_COMPARISONS_IDX;"
			"CREATE INDEX GENEALOGICAL_COMPARISONS_IDX ON GENEALOGICAL_COMPARISONS (PRIMARY_WIT, SECONDARY_WIT);";
	char * create_genealogical_comparisons_idx_error_msg;
	rc = sqlite3_exec(output_db, create_genealogical_comparisons_idx_sql.c_str(), NULL, 0, & create_genealogical_comparisons_idx_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating index GENEALOGICAL_COMPARISONS_IDX: " << create_genealogical_comparisons_idx_error_msg << endl;
		sqlite3_free(create_genealogical_comparisons_idx_error_msg);
		exit(1);
	}
	//Then populate it using prepared statements within a single transaction:
	char * transaction_error_msg;
	sqlite3_exec(output_db, "BEGIN TRANSACTION", NULL, NULL, & transaction_error_msg);
	sqlite3_stmt * insert_into_genealogical_comparisons_stmt;
	rc = sqlite3_prepare(output_db, "INSERT INTO GENEALOGICAL_COMPARISONS VALUES (?,?,?,?,?)", -1, & insert_into_genealogical_comparisons_stmt, 0);
	if (rc != SQLITE_OK) {
		cerr << "Error preparing statement." << endl;
		exit(1);
	}
	for (witness primary_wit : witnesses) {
		string primary_wit_id = primary_wit.get_id();
		unordered_map<string, genealogical_comparison> genealogical_comparisons = primary_wit.get_genealogical_comparisons();
		for (pair<string, genealogical_comparison> kv : genealogical_comparisons) {
			string secondary_wit_id = kv.first;
			genealogical_comparison comp = kv.second;
			//Serialize the bitmaps into byte arrays:
			Roaring agreements = comp.agreements;
			uint32_t agreements_expected_size = agreements.getSizeInBytes();
			char * agreements_buf = new char [agreements_expected_size];
			agreements.write(agreements_buf);
			Roaring explained = comp.explained;
			uint32_t explained_expected_size = explained.getSizeInBytes();
			char * explained_buf = new char [explained_expected_size];
			explained.write(explained_buf);
			//Get the genealogical cost:
			float cost = comp.cost;
			//Then insert a row containing these values:
			sqlite3_bind_text(insert_into_genealogical_comparisons_stmt, 1, primary_wit_id.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(insert_into_genealogical_comparisons_stmt, 2, secondary_wit_id.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_blob(insert_into_genealogical_comparisons_stmt, 3, agreements_buf, agreements_expected_size, SQLITE_STATIC);
			sqlite3_bind_blob(insert_into_genealogical_comparisons_stmt, 4, explained_buf, explained_expected_size, SQLITE_STATIC);
			sqlite3_bind_double(insert_into_genealogical_comparisons_stmt, 5, cost);
			rc = sqlite3_step(insert_into_genealogical_comparisons_stmt);
			if (rc != SQLITE_DONE) {
				cerr << "Error executing prepared statement." << endl;
				delete[] agreements_buf;
				delete[] explained_buf;
				exit(1);
			}
			//Then clean up allocated memory and reset the prepared statement so we can bind the next values to it:
			delete[] agreements_buf;
			delete[] explained_buf;
			sqlite3_reset(insert_into_genealogical_comparisons_stmt);
		}
	}
	sqlite3_finalize(insert_into_genealogical_comparisons_stmt);
	sqlite3_exec(output_db, "END TRANSACTION", NULL, NULL, & transaction_error_msg);
	return;
}

/**
 * Creates, indexes, and populates the WITNESSES table.
 */
void populate_witnesses_table(sqlite3 * output_db, const list<witness> & witnesses) {
	int rc; //to store SQLite macros
	cout << "Populating table WITNESSES..." << endl;
	//Create the WITNESSES table:
	string create_witnesses_sql = "DROP TABLE IF EXISTS WITNESSES;"
			"CREATE TABLE WITNESSES ("
			"WITNESS TEXT NOT NULL);";
	char * create_witnesses_error_msg;
	rc = sqlite3_exec(output_db, create_witnesses_sql.c_str(), NULL, 0, & create_witnesses_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating table WITNESSES: " << create_witnesses_error_msg << endl;
		sqlite3_free(create_witnesses_error_msg);
		exit(1);
	}
	//Denormalize it:
	string create_witnesses_idx_sql = "DROP INDEX IF EXISTS WITNESSES_IDX;"
			"CREATE INDEX WITNESSES_IDX ON WITNESSES (WITNESS);";
	char * create_witnesses_idx_error_msg;
	rc = sqlite3_exec(output_db, create_witnesses_idx_sql.c_str(), NULL, 0, & create_witnesses_idx_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating index WITNESSES_IDX: " << create_witnesses_idx_error_msg << endl;
		sqlite3_free(create_witnesses_idx_error_msg);
		exit(1);
	}
	//Then populate it using prepared statements within a single transaction:
	char * transaction_error_msg;
	sqlite3_exec(output_db, "BEGIN TRANSACTION", NULL, NULL, & transaction_error_msg);
	sqlite3_stmt * insert_into_witnesses_stmt;
	rc = sqlite3_prepare(output_db, "INSERT INTO WITNESSES VALUES (?)", -1, & insert_into_witnesses_stmt, 0);
	if (rc != SQLITE_OK) {
		cerr << "Error preparing statement." << endl;
		exit(1);
	}
	for (witness wit : witnesses) {
		string wit_id = wit.get_id();
		//Then insert a row containing these values:
		sqlite3_bind_text(insert_into_witnesses_stmt, 1, wit_id.c_str(), -1, SQLITE_STATIC);
		rc = sqlite3_step(insert_into_witnesses_stmt);
		if (rc != SQLITE_DONE) {
			cerr << "Error executing prepared statement." << endl;
			exit(1);
		}
		//Then reset the prepared statement so we can bind the next values to it:
		sqlite3_reset(insert_into_witnesses_stmt);
	}
	sqlite3_finalize(insert_into_witnesses_stmt);
	sqlite3_exec(output_db, "END TRANSACTION", NULL, NULL, & transaction_error_msg);
	return;
}

/**
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Read in the command-line options:
	set<string> trivial_reading_types = set<string>();
	set<string> dropped_reading_types = set<string>();
	bool merge_splits = false;
	int threshold = 0;
	string input_xml_name = string();
	string output_db_name = string();
	try {
		cxxopts::Options options("populate_db", "Parse the given collation XML file and populate the genealogical cache in the given SQLite database.");
		options.custom_help("[-h] [-t threshold] [-z trivial_reading_type_1 -z trivial_reading_type_2 ...] [-Z dropped_reading_type_1 -Z dropped_reading_type_2 ...] [--merge-splits] input_xml output_db");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("t,threshold", "minimum extant readings threshold", cxxopts::value<int>())
				("z", "reading type to treat as trivial (this may be used multiple times)", cxxopts::value<vector<string>>())
				("Z", "reading type to drop entirely (this may be used multiple times)", cxxopts::value<vector<string>>())
				("merge-splits", "merge split attestations of the same reading", cxxopts::value<bool>());
		options.add_options("positional")
				("input_xml", "collation file in TEI XML format", cxxopts::value<string>())
				("output_db", "output SQLite database (if an existing database is provided, its contents will be overwritten)", cxxopts::value<vector<string>>());
		options.parse_positional({"input_xml", "output_db"});
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
		if (args.count("z")) {
			for (string trivial_reading_type : args["z"].as<vector<string>>()) {
				trivial_reading_types.insert(trivial_reading_type);
			}
		}
		if (args.count("Z")) {
			for (string dropped_reading_type : args["Z"].as<vector<string>>()) {
				dropped_reading_types.insert(dropped_reading_type);
			}
		}
		if (args.count("merge-splits")) {
			merge_splits = args["merge-splits"].as<bool>();
		}
		//Parse the positional arguments:
		if (!args.count("input_xml") || args.count("output_db") != 1) {
			cerr << "Error: 2 positional arguments (input_xml and output_db) are required." << endl;
			exit(1);
		}
		else {
			input_xml_name = args["input_xml"].as<string>();
			output_db_name = args["output_db"].as<vector<string>>()[0];
		}
	}
	catch (const cxxopts::OptionException & e) {
		cerr << "Error parsing options: " << e.what() << endl;
		exit(-1);
	}
	//Attempt to parse the input XML file as an apparatus:
	pugi::xml_document doc;
	pugi::xml_parse_result pr = doc.load_file(input_xml_name.c_str());
	if (!pr) {
		cerr << "Error: An error occurred while loading XML file " << input_xml_name << ": " << pr.description() << endl;
		exit(1);
	}
	pugi::xml_node tei_node = doc.child("TEI");
	if (!tei_node) {
		cerr << "Error: The XML file " << input_xml_name << " does not have a <TEI> element as its root element." << endl;
		exit(1);
	}
	apparatus app = apparatus(tei_node, merge_splits, trivial_reading_types, dropped_reading_types);
	//Get the apparatus's list of variation units:
	list<variation_unit> variation_units = list<variation_unit>();
	for (variation_unit vu : app.get_variation_units()) {
		variation_units.push_back(vu);
	}
	//If the user has specified a minimum extant readings threshold,
	//then populate a set of witnesses that meet the threshold:
	list<string> list_wit = list<string>();
	if (threshold > 0) {
		cout << "Filtering out fragmentary witnesses... " << endl;
		for (string wit_id : app.get_list_wit()) {
			if (app.get_extant_passages_for_witness(wit_id) >= threshold) {
				list_wit.push_back(wit_id);
			}
		}
	}
	//Otherwise, just use the full list of witnesses found in the apparatus:
	else {
		list_wit = app.get_list_wit();
	}
	//Then initialize all of these witnesses:
	cout << "Initializing all witnesses (this may take a while)... " << endl;
	list<witness> witnesses = list<witness>();
	for (string wit_id : list_wit) {
		cout << "Calculating coherence for witness " << wit_id << "..." << endl;
		witness wit = witness(wit_id, list_wit, app);
		witnesses.push_back(wit);
	}
	//Now open the output database:
	cout << "Opening database..." << endl;
	sqlite3 * output_db;
	int rc = sqlite3_open(output_db_name.c_str(), & output_db);
	if (rc) {
		cerr << "Error opening database " << output_db_name << ": " << sqlite3_errmsg(output_db) << endl;
		exit(1);
	}
	//Populate each table:
	populate_readings_table(output_db, variation_units);
	populate_reading_relations_table(output_db, variation_units);
	populate_reading_support_table(output_db, variation_units);
	populate_variation_units_table(output_db, variation_units);
	populate_genealogical_comparisons_table(output_db, witnesses);
	populate_witnesses_table(output_db, witnesses);
	//Finally, close the output database:
	cout << "Closing database..." << endl;
	sqlite3_close(output_db);
	cout << "Database closed." << endl;
	exit(0);
}
