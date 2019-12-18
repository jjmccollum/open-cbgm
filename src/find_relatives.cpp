/*
 * find_relatives.cpp
 *
 *  Created on: Nov 26, 2019
 *      Author: jjmccollum
 */

#include <getopt.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <list>
#include <set>
#include <unordered_map>

#include "pugixml.h"
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
	int uncl; //number of variation units where both witnesses have different readings, but either's could explain that of the other
	int norel; //number of variation units where the primary witness has a reading unrelated to that of the secondary witness
};

/**
 * Prints the short usage message.
 */
void usage() {
	printf("usage: find_relatives [-h] [-t threshold] [--split] [--orth] [--def] input_xml witness passage\n\n");
	return;
}

/**
 * Prints the help message.
 */
void help() {
	usage();
	printf("Get a table of genealogical relationships between the witness with the given ID and other witnesses, as specified by the user.\n\n");
	printf("optional arguments:\n");
	printf("\t-h, --help: print usage manual\n");
	printf("\t-t, --threshold: minimum extant readings threshold\n");
	printf("\t-r, --reading: filter results for specific reading\n");
	printf("\t--split: treat split attestations as distinct readings\n");
	printf("\t--orth: treat orthographic subvariants as distinct readings\n");
	printf("\t--def: treat defective forms as distinct readings\n\n");
	printf("positional arguments:\n");
	printf("\tinput_xml: collation file in TEI XML format\n");
	printf("\twitness: ID of the witness whose relatives are desired, as found in its <witness> element in the XML file\n");
	printf("\tpassage: Label of the variation unit at which relatives' readings are desired\n");
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
	string filter_reading = "";
	const char* const short_opts = "ht:r:";
	const option long_opts[] = {
		{"split", no_argument, & split, 1},
		{"orth", no_argument, & orth, 1},
		{"def", no_argument, & def, 1},
		{"threshold", required_argument, nullptr, 't'},
		{"reading", required_argument, nullptr, 'r'},
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
			case 'r':
				filter_reading = optarg;
				break;
			case 0:
				//This will happen if a long option is being parsed; just move on:
				break;
			default:
				cout << "Error: invalid argument." << endl;
				usage();
				exit(1);
		}
	}
	//Parse the positional arguments:
	int index = optind;
	if (argc <= index + 2) {
		cout << "Error: 3 positional arguments (input_xml, witness, and passage) are required." << endl;
		exit(1);
	}
	//The first positional argument is the XML file:
	char * input_xml = argv[index];
	index++;
	//The next argument is the primary witness ID:
	string primary_wit_id = string(argv[index]);
	index++;
	//The remaining arguments are the tokens in the variation unit label:
	string vu_label = "";
	while (index < argc) {
		vu_label += argv[index];
		if (index + 1 < argc) {
			vu_label += " ";
		}
		index++;
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
	//Attempt to parse the input XML file as an apparatus:
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(input_xml);
	if (!result) {
		cout << "Error: An error occurred while loading XML file " << input_xml << ": " << result.description() << endl;
		exit(1);
	}
	pugi::xml_node tei_node = doc.child("TEI");
	if (!tei_node) {
		cout << "Error: The XML file " << input_xml << " does not have a <TEI> element as its root element." << endl;
		exit(1);
	}
	apparatus app = apparatus(tei_node, distinct_reading_types);
	//Attempt to retrieve the input variation unit:
	variation_unit vu;
	bool variation_unit_exists = false;
	for (variation_unit curr_vu : app.get_variation_units()) {
		if (curr_vu.get_label() == vu_label) {
			vu = curr_vu;
			variation_unit_exists = true;
			break;
		}
	}
	if (!variation_unit_exists) {
		cout << "Error: The XML file has no <app> element with a <label> value of " << vu_label << "." << endl;
		exit(1);
	}
	//Ensure that the primary witness is included in the apparatus's <listWit> element:
	bool primary_wit_exists = false;
	for (string wit_id : app.get_list_wit()) {
		if (wit_id == primary_wit_id) {
			primary_wit_exists = true;
			break;
		}
	}
	if (!primary_wit_exists) {
		cout << "Error: The XML file's <listWit> element has no child <witness> element with ID " << primary_wit_id << "." << endl;
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
	//Then initialize the primary witness:
	witness primary_wit = witness(primary_wit_id, list_wit, app);
	//Then populate a map of secondary witnesses, keyed by ID:
	unordered_map<string, witness> secondary_witnesses_by_id = unordered_map<string, witness>();
	for (string secondary_wit_id : list_wit) {
		//Skip the primary witness:
		if (secondary_wit_id == primary_wit_id) {
			continue;
		}
		//Initialize the secondary witness relative to the primary witness:
		list<string> secondary_list_wit = list<string>({primary_wit_id, secondary_wit_id});
		witness secondary_wit = witness(secondary_wit_id, secondary_list_wit, app);
		//Add it to the map:
		secondary_witnesses_by_id[secondary_wit_id] = secondary_wit;
	}
	cout << "Calculating relative comparisons for " << primary_wit_id << " at " << vu_label << "..." << endl;
	//Now calculate the comparison metrics between the primary witness and all of the secondary witnesses:
	list<witness_comparison> comparisons = list<witness_comparison>();
	unordered_map<string, list<string>> reading_support = vu.get_reading_support();
	Roaring primary_extant = primary_wit.get_explained_readings_for_witness(primary_wit_id);
	for (pair<string, witness> kv : secondary_witnesses_by_id) {
		string secondary_wit_id = kv.first;
		witness secondary_wit = kv.second;
		Roaring secondary_extant = secondary_wit.get_explained_readings_for_witness(secondary_wit_id);
		Roaring mutually_extant = primary_extant & secondary_extant;
		Roaring agreements = primary_wit.get_agreements_for_witness(secondary_wit_id);
		Roaring primary_explained_by_secondary = primary_wit.get_explained_readings_for_witness(secondary_wit_id);
		Roaring secondary_explained_by_primary = secondary_wit.get_explained_readings_for_witness(primary_wit_id);
		Roaring mutually_explained = primary_explained_by_secondary & secondary_explained_by_primary;
		witness_comparison comparison;
		comparison.id = secondary_wit_id;
		comparison.rdgs = list<string>();
		if (reading_support.find(secondary_wit_id) != reading_support.end()) {
			comparison.rdgs = reading_support[secondary_wit_id];
		}
		comparison.pass = mutually_extant.cardinality();
		comparison.eq = agreements.cardinality();
		comparison.prior = (secondary_explained_by_primary ^ mutually_explained).cardinality();
		comparison.posterior = (primary_explained_by_secondary ^ mutually_explained).cardinality();
		comparison.uncl = (mutually_explained ^ agreements).cardinality();
		comparison.norel = comparison.pass - comparison.eq - comparison.prior - comparison.posterior;
		comparison.perc = comparison.pass > 0 ? (100 * float(comparison.eq) / float(comparison.pass)) : 0;
		comparison.dir = comparison.prior > comparison.posterior ? -1 : (comparison.posterior > comparison.prior ? 1 : 0);
		comparisons.push_back(comparison);
	}
	//Sort the list of comparisons from highest number of agreements to lowest:
	comparisons.sort([](const witness_comparison & wc1, const witness_comparison & wc2) {
		return wc1.perc > wc2.perc;
	});
	//Pass through the sorted list of comparison to assign ancestral ranks:
	int nr = 1;
	for (witness_comparison & comparison : comparisons) {
		if (comparison.dir == 1) {
			comparison.nr = nr;
			nr++;
		}
		else if (comparison.dir == 0) {
			comparison.nr = 0;
		}
		else {
			comparison.nr = -1;
		}
	}
	cout << "Relatives of W1 = " << primary_wit_id << " at " << vu_label << ":";
	cout << "\n\n";
	cout << std::left << std::setw(8) << "W2";
	cout << std::left << std::setw(4) << "DIR";
	cout << std::right << std::setw(8) << "NR";
	cout << std::setw(4) << ""; //buffer space between right-aligned and left-aligned columns
	cout << std::left << std::setw(8) << "RDG";
	cout << std::right << std::setw(8) << "PASS";
	cout << std::right << std::setw(12) << "PERC";
	cout << std::right << std::setw(8) << "EQ";
	cout << std::right << std::setw(8) << "W1>W2";
	cout << std::right << std::setw(8) << "W1<W2";
	cout << std::right << std::setw(8) << "UNCL";
	cout << std::right << std::setw(8) << "NOREL";
	cout << "\n\n";
	for (witness_comparison comparison : comparisons) {
		string rdgs_str = "";
		bool match = filter_reading == "" ? true : false;
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
		if (rdgs_str == "") {
			rdgs_str = "-";
		}
		cout << std::left << std::setw(8) << comparison.id;
		cout << std::left << std::setw(4) << (comparison.dir == -1 ? "<" : (comparison.dir == 1 ? ">" : "="));
		cout << std::right << std::setw(8) << (comparison.nr > 0 ? to_string(comparison.nr) : "");
		cout << std::setw(4) << ""; //buffer space between right-aligned and left-aligned columns
		cout << std::left << std::setw(8) << rdgs_str;
		cout << std::right << std::setw(8) << comparison.pass;
		cout << std::right << std::setw(11) << fixed << std::setprecision(3) << comparison.perc << "%";
		cout << std::right << std::setw(8) << comparison.eq;
		cout << std::right << std::setw(8) << comparison.prior;
		cout << std::right << std::setw(8) << comparison.posterior;
		cout << std::right << std::setw(8) << comparison.uncl;
		cout << std::right << std::setw(8) << comparison.norel;
		cout << "\n";
	}
	cout << endl;
	exit(0);
}
