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
	//int uncl; //number of variation units where both witnesses have different readings, but either's could explain that of the other
	int norel; //number of variation units where the primary witness has a reading unrelated to that of the secondary witness
};

/**
 * Given primary witness ID, a variation unit label, a filter reading,
 * and a list of witness comparisons (assumed to be sorted in decreasing order of agreements),
 * prints the relatives list for the primary witness at the given variation unit.
 */
void print_relatives(const string & primary_wit_id, const string & vu_label, const string & filter_reading, const list<witness_comparison> & comparisons) {
	//Print the caption:
	if (filter_reading.empty()) {
		cout << "Relatives of W1 = " << primary_wit_id << " at " << vu_label << " ";
	}
	else {
		cout << "Relatives of W1 = " << primary_wit_id << " at " << vu_label << " with reading " << filter_reading << " ";
	}
	if (reading_support.find(primary_wit_id) != reading_support.end()) {
		cout << "(W1 RDG = ";
		list<string> primary_wit_rdgs = reading_support.at(primary_wit_id);
		for (string primary_wit_rdg : primary_wit_rdgs) {
			if (primary_wit_rdg != primary_wit_rdgs.front()) {
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
	int threshold = 0;
	string filter_reading = string();
	set<string> trivial_reading_types = set<string>();
	bool merge_splits = false;
	string input_xml = string();
	string primary_wit_id = string();
	string vu_id = string();
	try {
		cxxopts::Options options("find_relatives", "Get a table of genealogical relationships between the witness with the given ID and other witnesses at a given passage, as specified by the user.\nOptionally, the user can optionally specify a reading ID for the given passage, in which case the output will be restricted to the witnesses preserving that reading.");
		options.custom_help("[-h] [-t threshold] [-r reading] [-z trivial_reading_types] [--merge-splits] input_xml witness passage");
		//options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("t,threshold", "minimum extant readings threshold", cxxopts::value<int>())
				("r,reading", "ID of desired variant reading", cxxopts::value<string>())
				("z", "space-separated list of reading types to treat as trivial (e.g., defective orthographic)", cxxopts::value<vector<string>>())
				("merge-splits", "merge split attestations of the same reading", cxxopts::value<bool>());
		options.add_options("positional")
				("input_xml", "collation file in TEI XML format", cxxopts::value<string>())
				("witness", "ID of the witness whose relatives are desired, as found in its <witness> element in the XML file", cxxopts::value<string>())
				("passage", "ID or index (0-based) of the variation unit at which relatives' readings are desired", cxxopts::value<vector<string>>());
		options.parse_positional({"input_xml", "witness", "passage"});
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
		if (args.count("r")) {
			filter_reading = args["r"].as<string>();
		}
		if (args.count("z")) {
			for (string trivial_reading_type : args["z"].as<vector<string>>()) {
				trivial_reading_types.insert(trivial_reading_type);
			}
		}
		if (args.count("merge-splits")) {
			merge_splits = args["merge-splits"].as<bool>();
		}
		//Parse the positional arguments:
		if (!args.count("input_xml") || !args.count("witness") || args.count("passage") != 1) {
			cerr << "Error: 3 positional arguments (input_xml, witness, and passage) are required." << endl;
			exit(1);
		}
		else {
			input_xml = args["input_xml"].as<string>();
			primary_wit_id = args["witness"].as<string>();
			vu_id = args["passage"].as<vector<string>>()[0];
		}
	}
	catch (const cxxopts::OptionException & e) {
		cerr << "Error parsing options: " << e.what() << endl;
		exit(-1);
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
	apparatus app = apparatus(tei_node, merge_splits, trivial_reading_types);
	//Attempt to retrieve the input variation unit by searching for a match in the apparatus:
	variation_unit vu;
	bool variation_unit_matched = false;
	for (variation_unit curr_vu : app.get_variation_units()) {
		if (curr_vu.get_id() == vu_id) {
			vu = curr_vu;
			variation_unit_matched = true;
			break;
		}
	}
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
	    	cerr << "Error: The XML file has no <app> element with an xml:id, id, or n attribute value of " << vu_id << "." << endl;
			exit(1);
	    }
	    //Otherwise, convert the ID to a number, and check if it is a valid index:
	    unsigned int vu_ind = atoi(vu_id.c_str());
	    if (vu_ind >= app.get_variation_units().size()) {
	    	cerr << "Error: The XML file has no <app> element with an xml:id, id, or n attribute value of " << vu_id << "; if the variation unit ID was specified as an index, then it is out of range, as there are only " << app.get_variation_units().size() << " variation units." << endl;
			exit(1);
	    }
	    //Otherwise, get the variation unit corresponding to this ID:
	    vu = app.get_variation_units()[vu_ind];
	    variation_unit_matched = true;
	}
	//Get the label for this variation unit, if it exists; otherwise, use the ID:
	string vu_label = vu.get_label().empty() ? vu_id : vu.get_label();
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
	cout << "Calculating genealogical relationships between witness " << primary_wit_id << " and all other witnesses..." << endl;
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
	cout << "Sorting relatives for " << primary_wit_id << " at " << vu_label << "..." << endl;
	//Now calculate the comparison metrics between the primary witness and all of the secondary witnesses:
	list<witness_comparison> comparisons = list<witness_comparison>();
	unordered_map<string, list<string>> reading_support = vu.get_reading_support();
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
		comparisons.push_back(comparison);
	}
	//Sort the list of comparisons from highest number of agreements to lowest:
	comparisons.sort([](const witness_comparison & wc1, const witness_comparison & wc2) {
		return wc1.eq > wc2.eq;
	});
	//Pass through the sorted list of comparison to assign ancestral ranks:
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
	print_relatives(primary_wit_id, vu_label, filter_reading, comparisons);
	exit(0);
}
