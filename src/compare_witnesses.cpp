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

#include "cxxopts.h"
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
	//int uncl; //number of variation units where both witnesses have different readings, but either's could explain that of the other
	int norel; //number of variation units where the primary witness has a reading unrelated to that of the secondary witness
};

/**
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Read in the command-line options:
	bool split = false;
	bool orth = false;
	bool def = false;
	int threshold = 0;
	string input_xml = string();
	string primary_wit_id = string();
	set<string> secondary_wit_ids = set<string>();
	list<string> secondary_wit_ids_ordered = list<string>(); //for printing purposes
	try {
		cxxopts::Options options("compare_witnesses", "Get a table of genealogical relationships between the witness with the given ID and other witnesses, as specified by the user.");
		options.custom_help("[-h] [-t threshold] [--split] [--orth] [--def] input_xml witness [secondary_witnesses]");
		//options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("t,threshold", "minimum extant readings threshold", cxxopts::value<int>())
				("split", "treat split attestations as distinct readings", cxxopts::value<bool>())
				("orth", "treat orthographic subvariants as distinct readings", cxxopts::value<bool>())
				("def", "treat defective forms as distinct readings", cxxopts::value<bool>());
		options.add_options("positional arguments")
				("input_xml", "collation file in TEI XML format", cxxopts::value<string>())
				("witness", "ID of the primary witness to be compared, as found in its <witness> element in the XML file", cxxopts::value<string>())
				("secondary_witnesses", "IDs of secondary witness to be compared to the primary witness (if not specified, then the primary witness will be compared to all other witnesses)", cxxopts::value<vector<string>>());
		options.parse_positional({"input_xml", "witness", "secondary_witnesses"});
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
		//Parse the positional arguments:
		if (!args.count("input_xml") || !args.count("witness")) {
			cerr << "Error: At least 2 positional arguments (input_xml and witness) are required." << endl;
			exit(1);
		}
		else {
			input_xml = args["input_xml"].as<string>();
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
	pugi::xml_parse_result result = doc.load_file(input_xml.c_str());
	if (!result) {
		cerr << "Error: An error occurred while loading XML file " << input_xml << ": " << result.description() << endl;
		exit(1);
	}
	pugi::xml_node tei_node = doc.child("TEI");
	if (!tei_node) {
		cout << "Error: The XML file " << input_xml << " does not have a <TEI> element as its root element." << endl;
		exit(1);
	}
	apparatus app = apparatus(tei_node, distinct_reading_types);
	//Ensure that the primary witness is included in the apparatus's <listWit> element:
	bool primary_wit_exists = false;
	for (string wit_id : app.get_list_wit()) {
		if (wit_id == primary_wit_id) {
			primary_wit_exists = true;
			break;
		}
	}
	if (!primary_wit_exists) {
		cerr << "Error: The XML file's <listWit> element has no child <witness> element with ID " << primary_wit_id << "." << endl;
		exit(1);
	}
	//If the user has specified a set of desired secondary witnesses or a minimum extant readings threshold,
	//then populate a list of witnesses that meet these criteria:
	list<string> list_wit = list<string>();
	if (threshold > 0 || !secondary_wit_ids.empty()) {
		cout << "Filtering witnesses... " << endl;
		for (string wit_id : app.get_list_wit()) {
			//Skip fragmentary witnesses:
			if (threshold > 0 && app.get_extant_passages_for_witness(wit_id) < threshold) {
				//If the primary witness is fragmentary, then report this to the user and exit:
				if (wit_id == primary_wit_id) {
					cout << "Primary witness " << primary_wit_id << " does not meet the specified minimum extant readings threshold of " << threshold << "." << endl;
					exit(0);
				}
				continue;
			}
			//If a set of desired secondary witnesses was specified, then skip any witnesses not contained in it:
			if (!secondary_wit_ids.empty() && secondary_wit_ids.find(wit_id) == secondary_wit_ids.end()) {
				//Except for the primary witness:
				if (wit_id == primary_wit_id) {
					list_wit.push_back(wit_id);
				}
				continue;
			}
			//Otherwise, add this witness to the list:
			list_wit.push_back(wit_id);
		}
	}
	//Otherwise, just use the full list of witnesses found in the apparatus:
	else {
		list_wit = app.get_list_wit();
	}
	cout << "Calculating genealogical relationships between witness " << primary_wit_id;
	if (secondary_wit_ids.empty()) {
		cout << " and all other witnesses...";
	}
	else {
		cout << " and witness(es) ";
		for (string secondary_wit_id : secondary_wit_ids_ordered) {
			if (secondary_wit_id != secondary_wit_ids_ordered.front()) {
				cout << ", ";
			}
			cout << secondary_wit_id;
		}
		cout << "...";
	}
	cout << endl;
	//Then initialize the primary witness:
	witness primary_wit = witness(primary_wit_id, list_wit, app);
	//Then populate a list of secondary witnesses:
	list<witness> secondary_witnesses = list<witness>();
	for (string secondary_wit_id : list_wit) {
		//Skip the primary witness:
		if (secondary_wit_id == primary_wit_id) {
			continue;
		}
		//Initialize the secondary witness relative to the primary witness:
		list<string> secondary_list_wit = list<string>({primary_wit_id, secondary_wit_id});
		witness secondary_wit = witness(secondary_wit_id, secondary_list_wit, app);
		//Add it to the list:
		secondary_witnesses.push_back(secondary_wit);
	}
	//Now calculate the comparison metrics between the primary witness and all of the secondary witnesses:
	list<witness_comparison> comparisons = list<witness_comparison>();
	for (witness secondary_wit : secondary_witnesses) {
		string secondary_wit_id = secondary_wit.get_id();
		genealogical_comparison primary_primary_comp = primary_wit.get_genealogical_comparison_for_witness(primary_wit_id);
		genealogical_comparison primary_secondary_comp = primary_wit.get_genealogical_comparison_for_witness(secondary_wit_id);
		genealogical_comparison secondary_primary_comp = secondary_wit.get_genealogical_comparison_for_witness(primary_wit_id);
		genealogical_comparison secondary_secondary_comp = secondary_wit.get_genealogical_comparison_for_witness(secondary_wit_id);
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
		comparisons.push_back(comparison);
	}
	//Sort the list of comparisons from highest number of agreements to lowest:
	comparisons.sort([](const witness_comparison & wc1, const witness_comparison & wc2) {
		return wc1.perc > wc2.perc ? true : (wc1.perc < wc2.perc ? false : false);
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
		cout << std::right << std::setw(8) << comparison.norel;
		cout << "\n";
	}
	cout << endl;
	exit(0);
}
