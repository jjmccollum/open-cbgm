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
#include <getopt.h>

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
 * Prints the short usage message.
 */
void usage() {
	printf("usage: print_graphs [-h] [-t threshold] [--split] [--orth] [--def] [--local] [--flow] [--attestations] [--variants] [--global] input_xml\n\n");
	return;
}

/**
 * Prints the help message.
 */
void help() {
	usage();
	printf("Prints diagrams of CBGM graphs to .dot output files. The output files will be organized in separate directories based on diagram type.\n\n");
	printf("optional arguments:\n");
	printf("\t-h, --help: print usage manual\n");
	printf("\t-t, --threshold: minimum extant readings threshold\n");
	printf("\t--split: treat split attestations as distinct readings\n");
	printf("\t--orth: treat orthographic subvariants as distinct readings\n");
	printf("\t--def: treat defective forms as distinct readings\n");
	printf("\t--local: print local stemmata diagrams\n");
	printf("\t--flow: print complete textual flow diagrams for all passages\n");
	printf("\t--attestations: print coherence in attestation textual flow diagrams for all readings at all passages\n");
	printf("\t--variants: print coherence at variant passages diagrams (i.e., textual flow diagrams restricted to flow between different readings) at all passages\n");
	printf("\t--global: print global stemma diagram (this may take several minutes)\n\n");
	printf("positional arguments:\n");
	printf("\tinput_xml: collation file in TEI XML format\n");
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
	int threshold = 0;
	int local = 0;
	int flow = 0;
	int attestations = 0;
	int variants = 0;
	int global = 0;
	const char* const short_opts = "ht:";
	const option long_opts[] = {
		{"split", no_argument, & split, 1},
		{"orth", no_argument, & orth, 1},
		{"def", no_argument, & def, 1},
		{"threshold", required_argument, nullptr, 't'},
		{"local", no_argument, & local, 1},
		{"flow", no_argument, & flow, 1},
		{"attestations", no_argument, & attestations, 1},
		{"variants", no_argument, & variants, 1},
		{"global", no_argument, & global, 1},
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
			case 0:
				//This will happen if a long option is being parsed; just move on:
				break;
			default:
				cerr << "Error: invalid argument." << endl;
				usage();
				exit(1);
		}
	}
	//If no graphs are specified for printing, then print all of them:
	if (local + flow + attestations + variants + global == 0) {
		local = 1;
		flow = 1;
		attestations = 1;
		variants = 1;
		global = 1;
	}
	//Parse the positional arguments:
	int index = optind;
	if (argc <= index) {
		cerr << "Error: 1 positional argument (input_xml) is required." << endl;
		exit(1);
	}
	//The first positional argument is the XML file:
	char * input_xml = argv[index];
	index++;
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
	//Attempt to parse the input XML file as an apparatus:
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(input_xml);
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
	if (flow + attestations + variants + global == 0) {
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
	if (flow + attestations + variants > 0) {
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
			string filepath = flow_dir = "/" + filename;
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
	if (global == 0) {
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
