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
#include <unordered_set>
#include <unordered_map>
#include <getopt.h>

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
	printf("usage: compare_witnesses [-h] [-t threshold] [--split] [--orth] [--def] input_xml witness_1 [witness_2 ... witness_n]\n\n");
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
	printf("\t-t, --threshold: minimum extant readings threshold\n\n");
	printf("\t--split: treat split attestations as distinct readings");
	printf("\t--orth: treat orthographic subvariants as distinct readings");
	printf("\t--def: treat defective forms as distinct readings");
	printf("positional arguments:\n");
	printf("\tinput_xml: collation file in TEI XML format\n");
	printf("\twitness_1: ID of the primary witness to be compared, as found in its <witness> element in the XML file\n");
	printf("\twitness_2 ... witness_n: IDs of secondary witness to be compared to the primary witness (if not specified, then the primary witness will be compared to all other witnesses)\n\n");
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
	unsigned int threshold = 0;
	const char* const short_opts = "ht:";
	const option long_opts[] = {
		{"split", no_argument, & split, 1},
		{"orth", no_argument, & orth, 1},
		{"def", no_argument, & def, 1},
		{"threshold", required_argument, nullptr, 't'},
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
				printf("Error: invalid argument.\n");
				usage();
				exit(1);
		}
	}
	//Parse the positional arguments:
	int index = optind;
	if (argc <= index + 1) {
		printf("Error: At least 2 positional arguments (input_xml and witness_1) are required.\n");
		exit(1);
	}
	//The first positional argument is the XML file:
	char * input_xml = argv[index];
	index++;
	//The next argument is the primary witness ID:
	string primary_wit_id = string(argv[index]);
	index++;
	//The remaining arguments are IDs of specific secondary witnesses to consider;
	//if they are not specified, then we will include all witnesses:
	unordered_set<string> secondary_wit_ids = unordered_set<string>();
	for (int i = index; i < argc; i++) {
		string secondary_wit_id = string(argv[i]);
		secondary_wit_ids.insert(secondary_wit_id);
	}
	cout << "Calculating genealogical relationships between witness " << primary_wit_id;
	if (secondary_wit_ids.size() == 0) {
		cout << " and all other witnesses...";
	}
	else {
		cout << " and witness(es)";
		for (string secondary_wit_id : secondary_wit_ids) {
			cout << " " << secondary_wit_id;
		}
		cout << "...";
	}
	cout << endl;
	//Using the input flags, populate a set of reading types to be treated as distinct:
	unordered_set<string> distinct_reading_types = unordered_set<string>();
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
		printf("Error: An error occurred while loading XML file %s: %s\n", input_xml, result.description());
		exit(1);
	}
	pugi::xml_node tei_node = doc.child("TEI");
	if (!tei_node) {
		printf("Error: The XML file %s does not have a <TEI> element as its root element.\n", input_xml);
		exit(1);
	}
	apparatus app = apparatus(tei_node, distinct_reading_types);
	//Ensure that the primary witness is included in the apparatus's <listWit> element:
	unordered_set<string> list_wit = app.get_list_wit();
	if (list_wit.find(primary_wit_id) == list_wit.end()) {
		printf("Error: The XML file's <listWit> element has no child <witness> element with ID %s.\n", primary_wit_id.c_str());
		exit(1);
	}
	//If the user has not specified a set of secondary witness IDs, then set this to the apparatus's set of witness IDs:
	if (secondary_wit_ids.size() == 0) {
		secondary_wit_ids = unordered_set<string>(list_wit);
	}
	//Add the primary witness's ID to this set:
	secondary_wit_ids.insert(primary_wit_id);
	//Initialize the primary witness using the apparatus:
	witness primary_wit = witness(primary_wit_id, secondary_wit_ids, app);
	//Initialize the secondary witnesses according to the input parameters:
	unordered_map<string, witness> secondary_witnesses_by_id = unordered_map<string, witness>();
	for (string secondary_wit_id : list_wit) {
		//Skip the primary witness:
		if (secondary_wit_id == primary_wit_id) {
			continue;
		}
		//Skip any witnesses not in the input set:
		if (secondary_wit_ids.find(secondary_wit_id) == secondary_wit_ids.end()) {
			continue;
		}
		//Initialize a witness relative to the primary witness:
		unordered_set<string> wit_ids = unordered_set<string>({primary_wit_id, secondary_wit_id});
		witness secondary_wit = witness(secondary_wit_id, wit_ids, app);
		//If it is too lacunose, then ignore it:
		if (secondary_wit.get_explained_readings_for_witness(secondary_wit_id).cardinality() < threshold) {
			continue;
		}
		//Otherwise, add it to the map:
		secondary_witnesses_by_id[secondary_wit_id] = secondary_wit;
	}
	//Now calculate the comparison metrics between the primary witness and all of the secondary witnesses:
	list<witness_comparison> comparisons = list<witness_comparison>();
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
	cout << "Genealogical comparisons for W1 = " << primary_wit_id << ":";
	cout << "\n\n";
	cout << std::left << std::setw(8) << "W2";
	cout << std::left << std::setw(4) << "DIR";
	cout << std::right << std::setw(8) << "PASS";
	cout << std::right << std::setw(12) << "PERC";
	cout << std::right << std::setw(8) << "EQ";
	cout << std::right << std::setw(8) << "W1>W2";
	cout << std::right << std::setw(8) << "W1<W2";
	cout << std::right << std::setw(8) << "UNCL";
	cout << std::right << std::setw(8) << "NOREL";
	cout << "\n\n";
	for (witness_comparison comparison : comparisons) {
		cout << std::left << std::setw(8) << comparison.id;
		cout << std::left << std::setw(4) << (comparison.dir == -1 ? "<" : (comparison.dir == 1 ? ">" : "="));
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
	return 0;
}
