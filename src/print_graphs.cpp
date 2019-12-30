/*
 * print_graphs.cpp
 *
 *  Created on: Dec 13, 2019
 *      Author: jjmccollum
 */

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
#include "pugixml.h"
#include "roaring.hh"
#include "global_stemma.h"
#include "textual_flow.h"
#include "witness.h"
#include "apparatus.h"
#include "variation_unit.h"
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
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Read in the command-line options:
	bool split = false;
	bool orth = false;
	bool def = false;
	bool local = false;
	bool flow = false;
	bool attestations = false;
	bool variants = false;
	bool global = false;
	int threshold = 0;
	string input_xml = string();
	try {
		cxxopts::Options options("print_graphs", "Prints diagrams of CBGM graphs to .dot output files. The output files will be organized in separate directories based on diagram type.");
		options.custom_help("[-h] [-t threshold] [--split] [--orth] [--def] [--local] [--flow] [--attestations] [--variants] [--global] input_xml");
		//options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("t,threshold", "minimum extant readings threshold", cxxopts::value<int>())
				("split", "treat split attestations as distinct readings", cxxopts::value<bool>())
				("orth", "treat orthographic subvariants as distinct readings", cxxopts::value<bool>())
				("def", "treat defective forms as distinct readings", cxxopts::value<bool>())
				("local", "print local stemmata diagrams", cxxopts::value<bool>())
				("flow", "print complete textual flow diagrams for all passages", cxxopts::value<bool>())
				("attestations", "print coherence in attestation textual flow diagrams for all readings at all passages", cxxopts::value<bool>())
				("variants", "print coherence at variant passages diagrams (i.e., textual flow diagrams restricted to flow between different readings) at all passages", cxxopts::value<bool>())
				("global", "print global stemma diagram (this may take a while)", cxxopts::value<bool>());
		options.add_options("positional")
				("input_xml", "collation file in TEI XML format", cxxopts::value<vector<string>>());
		options.parse_positional({"input_xml"});
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
		if (args.count("split")) {
			split = args["split"].as<bool>();
		}
		if (args.count("orth")) {
			split = args["orth"].as<bool>();
		}
		if (args.count("def")) {
			split = args["def"].as<bool>();
		}
		if (args.count("local")) {
			local = args["local"].as<bool>();
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
		if (args.count("global")) {
			global = args["global"].as<bool>();
		}
		//Parse the positional arguments:
		if (args.count("input_xml") != 1) {
			cerr << "Error: 1 positional argument (input_xml) is required." << endl;
			exit(1);
		}
		else {
			input_xml = args["input_xml"].as<vector<string>>()[0];
		}
	}
	catch (const cxxopts::OptionException & e) {
		cerr << "Error parsing options: " << e.what() << endl;
		exit(-1);
	}
	//Using the input flags, populate a set of reading types to be treated as distinct:
	set<string> distinct_reading_types = set<string>();
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
	//If no graphs are specified for printing, then print all of them:
	if (!(local || flow || attestations || variants || global)) {
		local = true;
		flow = true;
		attestations = true;
		variants = true;
		global = true;
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
	apparatus app = apparatus(tei_node, distinct_reading_types);
	//If specified, print all local stemmata:
	if (local) {
		cout << "Printing all local stemmata..." << endl;
		//Create the directory to write files to:
		string local_dir = "local";
		create_dir(local_dir);
		int vu_ind = 0;
		for (variation_unit & vu : app.get_variation_units()) {
			string filename;
			//The filename base will be the ID of the variation unit, if it exists:
			if (!vu.get_id().empty()) {
				filename = vu.get_id();
			}
			//Otherwise, it will be the current variation unit index:
			else {
				filename = to_string(vu_ind);
			}
			filename += "-local-stemma.dot";
			//Complete the path to this file:
			string filepath = local_dir + "/" + filename;
			//Open a filestream:
			fstream dot_file;
			dot_file.open(filepath, ios::out);
			local_stemma ls = vu.get_local_stemma();
			ls.to_dot(dot_file);
			dot_file.close();
			vu_ind++;
		}
	}
	//If no other flags are set, then we're done:
	if (!(flow || attestations || variants || global)) {
		exit(0);
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
		cout << "Calculating coherences for witness " << wit_id << "..." << endl;
		witness wit = witness(wit_id, list_wit, app);
		witnesses.push_back(wit);
	}
	//Then populate each witness's list of potential ancestors:
	for (witness & wit : witnesses) {
		wit.set_potential_ancestor_ids(witnesses);
	}
	//If any type of textual flow diagrams are requested, then construct the textual flow diagrams for all variation units:
	vector<textual_flow> tfs = vector<textual_flow>();
	if (!(flow || attestations || variants)) {
		cout << "Calculating textual flow for all variation units..." << endl;
		for (variation_unit vu : app.get_variation_units()) {
			textual_flow tf = textual_flow(vu, witnesses);
			tfs.push_back(tf);
		}
	}
	//If specified, print all complete textual flow diagrams:
	if (flow) {
		cout << "Printing all complete textual flow diagrams..." << endl;
		//Create the directory to write files to:
		string flow_dir = "flow";
		create_dir(flow_dir);
		int vu_ind = 0;
		for (variation_unit vu : app.get_variation_units()) {
			string filename;
			//The filename base will be the ID of the variation unit, if it exists:
			if (!vu.get_id().empty()) {
				filename = vu.get_id();
			}
			//Otherwise, it will be the current variation unit index:
			else {
				filename = to_string(vu_ind);
			}
			filename += "-textual-flow.dot";
			//Complete the path to this file:
			string filepath = flow_dir + "/" + filename;
			//Open a filestream:
			fstream dot_file;
			dot_file.open(filepath, ios::out);
			textual_flow tf = tfs[vu_ind];
			tf.textual_flow_to_dot(dot_file);
			dot_file.close();
			vu_ind++;
		}
	}
	//If specified, print all coherence in attestations textual flow diagrams:
	if (attestations) {
		cout << "Printing all coherence in attestations textual flow diagrams..." << endl;
		//Create the directory to write files to:
		string attestations_dir = "attestations";
		create_dir(attestations_dir);
		int vu_ind = 0;
		for (variation_unit vu : app.get_variation_units()) {
			for (string rdg : vu.get_readings()) {
				string filename;
				//The filename base will be the ID of the variation unit, if it exists:
				if (!vu.get_id().empty()) {
					filename = vu.get_id();
				}
				//Otherwise, it will be the current variation unit index:
				else {
					filename = to_string(vu_ind);
				}
				filename += "R" + rdg; // prefix the reading with "R"
				filename += "-coherence-attestations.dot";
				//Complete the path to this file:
				string filepath = attestations_dir + "/" + filename;
				//Open a filestream:
				fstream dot_file;
				dot_file.open(filepath, ios::out);
				textual_flow tf = tfs[vu_ind];
				tf.coherence_in_attestations_to_dot(rdg, dot_file);
				dot_file.close();
			}
			vu_ind++;
		}
	}
	//If specified, print all coherence in variant passages flow diagrams:
	if (variants) {
		cout << "Printing all coherence in variant passages textual flow diagrams..." << endl;
		//Create the directory to write files to:
		string variants_dir = "variants";
		create_dir(variants_dir);
		int vu_ind = 0;
		for (variation_unit vu : app.get_variation_units()) {
			string filename;
			//The filename base will be the ID of the variation unit, if it exists:
			if (!vu.get_id().empty()) {
				filename = vu.get_id();
			}
			//Otherwise, it will be the current variation unit index:
			else {
				filename = to_string(vu_ind);
			}
			filename += "-coherence-variants.dot";
			//Complete the path to this file:
			string filepath = variants_dir + "/" + filename;
			//Open a filestream:
			fstream dot_file;
			dot_file.open(filepath, ios::out);
			textual_flow tf = tfs[vu_ind];
			tf.coherence_in_variant_passages_to_dot(dot_file);
			dot_file.close();
			vu_ind++;
		}
	}
	//If no other flags are set, then we're done:
	if (!global) {
		exit(0);
	}
	//Otherwise, optimize the substemmata for all witnesses:
	cout << "Optimizing substemmata for all witnesses (this may take a while)..." << endl;
	for (witness & wit: witnesses) {
		string wit_id = wit.get_id();
		cout << "Optimizing substemmata for witness " << wit_id << "..." << endl;
		wit.set_global_stemma_ancestor_ids();
	}
	//Then initialize the global stemma:
	global_stemma gs = global_stemma(witnesses);
	//If specified, print the global stemma:
	if (global) {
		cout << "Printing global stemma..." << endl;
		//Create the directory to write files to:
		string global_dir = "global";
		create_dir(global_dir);
		string filename = "global-stemma.dot";
		//Complete the path to this file:
		string filepath = global_dir + "/" + filename;
		//Open a filestream:
		fstream dot_file;
		dot_file.open(filepath, ios::out);
		gs.to_dot(dot_file);
		dot_file.close();
	}
	exit(0);
}
