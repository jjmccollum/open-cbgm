/*
 * enumerate_relationships_table.cpp
 *
 *  Created on: Feb 18, 2024
 *      Author: jjmccollum
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <list>
#include <vector>
#include <set>

#include "witness.h"
#include "enumerate_relationships_table.h"

using namespace std;

 /**
 * Default constructor.
 */
enumerate_relationships_table::enumerate_relationships_table() {

}

/**
 * Constructs an enumerate relationships table for a given genealogical comparison,
 * given a genealogical comparison and a vector of variation unit IDs.
 */
 enumerate_relationships_table::enumerate_relationships_table(const genealogical_comparison & comp, const vector<string> & variation_unit_ids) {
	//Copy in the primary and secondary witness IDs first:
    primary_wit_id = comp.primary_wit;
	secondary_wit_id = comp.secondary_wit;
	//Initialize a placeholder for iterating through each set index in a genealogical comparison bitmap:
	uint32_t vu_index;
	//Then populate the lists of variation unit IDs for each type of genealogical relationship:
	extant = list<string>();
	uint64_t extant_cardinality = comp.extant.cardinality();
	for (uint32_t i = 0; i < extant_cardinality; i++) {
    	comp.extant.select(i, &vu_index);
		extant.push_back(variation_unit_ids[vu_index]);
	}
	agreements = list<string>();
	uint64_t agreements_cardinality = comp.agreements.cardinality();
	for (uint32_t i = 0; i < agreements_cardinality; i++) {
    	comp.agreements.select(i, &vu_index);
		agreements.push_back(variation_unit_ids[vu_index]);
	}
	prior = list<string>();
	uint64_t prior_cardinality = comp.prior.cardinality();
	for (uint32_t i = 0; i < prior_cardinality; i++) {
    	comp.prior.select(i, &vu_index);
		prior.push_back(variation_unit_ids[vu_index]);
	}
	posterior = list<string>();
	uint64_t posterior_cardinality = comp.posterior.cardinality();
	for (uint32_t i = 0; i < posterior_cardinality; i++) {
    	comp.posterior.select(i, &vu_index);
		posterior.push_back(variation_unit_ids[vu_index]);
	}
	norel = list<string>();
	uint64_t norel_cardinality = comp.norel.cardinality();
	for (uint32_t i = 0; i < norel_cardinality; i++) {
    	comp.norel.select(i, &vu_index);
		norel.push_back(variation_unit_ids[vu_index]);
	}
	unclear = list<string>();
	uint64_t unclear_cardinality = comp.unclear.cardinality();
	for (uint32_t i = 0; i < unclear_cardinality; i++) {
    	comp.unclear.select(i, &vu_index);
		unclear.push_back(variation_unit_ids[vu_index]);
	}
	explained = list<string>();
	uint64_t explained_cardinality = comp.explained.cardinality();
	for (uint32_t i = 0; i < explained_cardinality; i++) {
    	comp.explained.select(i, &vu_index);
		explained.push_back(variation_unit_ids[vu_index]);
	}
}

/**
 * Default destructor.
 */
enumerate_relationships_table::~enumerate_relationships_table() {

}

/**
 * Returns the ID of the primary witness under comparison in this table.
 */
string enumerate_relationships_table::get_primary_wit_id() const {
    return primary_wit_id;
}

/**
 * Returns the ID of the secondary witness under comparison in this table.
 */
string enumerate_relationships_table::get_secondary_wit_id() const {
    return secondary_wit_id;
}

/**
 * Returns this table's list of passages where both witnesses are extant.
 */
list<string> enumerate_relationships_table::get_extant() const {
	return extant;
}

/**
 * Returns this table's list of passages where both witnesses agree.
 */
list<string> enumerate_relationships_table::get_agreements() const {
	return agreements;
}

/**
 * Returns this table's list of passages where the primary witness is prior.
 */
list<string> enumerate_relationships_table::get_prior() const {
	return prior;
}

/**
 * Returns this table's list of passages where the primary witness is posterior.
 */
list<string> enumerate_relationships_table::get_posterior() const {
	return posterior;
}

/**
 * Returns this table's list of passages where both witnesses' readings are known to have no directed relationship.
 */
list<string> enumerate_relationships_table::get_norel() const {
	return norel;
}

/**
 * Returns this table's list of passages where the witnesses' readings have an unclear relationship.
*/
list<string> enumerate_relationships_table::get_unclear() const {
	return unclear;
}

/**
 * Returns this table's list of passages where the primary witness has a reading explained by that of the secondary witness.
*/
list<string> enumerate_relationships_table::get_explained() const {
	return explained;
}

/**
 * Given an output stream and a set of desired relationship types, 
 * prints the contents of this table for those relationship types in fixed-width format.
 */
void enumerate_relationships_table::to_fixed_width(ostream & out, const set<string> & filter_relationship_types) {
    //Print the caption:
	out << "Genealogical relationships between " << primary_wit_id << " and " << secondary_wit_id << ":";
	out << "\n\n";
	//Then print the lists of passages for the filtered relationship types:
	if (filter_relationship_types.find("extant") != filter_relationship_types.end()) {
		out << "EXTANT";
		out << "\n\n";
		for (string vu_id : extant) {
			out << "\t" << vu_id;
			out << "\n";
		}
	}
	if (filter_relationship_types.find("agree") != filter_relationship_types.end()) {
		out << "AGREE";
		out << "\n\n";
		for (string vu_id : agreements) {
			out << "\t" << vu_id;
			out << "\n";
		}
	}
	if (filter_relationship_types.find("prior") != filter_relationship_types.end()) {
		out << "PRIOR";
		out << "\n\n";
		for (string vu_id : prior) {
			out << "\t" << vu_id;
			out << "\n";
		}
	}
	if (filter_relationship_types.find("posterior") != filter_relationship_types.end()) {
		out << "POSTERIOR";
		out << "\n\n";
		for (string vu_id : posterior) {
			out << "\t" << vu_id;
			out << "\n";
		}
	}
	if (filter_relationship_types.find("norel") != filter_relationship_types.end()) {
		out << "NOREL";
		out << "\n\n";
		for (string vu_id : norel) {
			out << "\t" << vu_id;
			out << "\n";
		}
	}
	if (filter_relationship_types.find("unclear") != filter_relationship_types.end()) {
		out << "UNCLEAR";
		out << "\n\n";
		for (string vu_id : unclear) {
			out << "\t" << vu_id;
			out << "\n";
		}
	}
	if (filter_relationship_types.find("explained") != filter_relationship_types.end()) {
		out << "EXPLAINED";
		out << "\n\n";
		for (string vu_id : explained) {
			out << "\t" << vu_id;
			out << "\n";
		}
	}
	out << endl;
	return;
}

/**
 * Given an output stream and a set of desired relationship types, 
 * prints the contents of this table for those relationship types in comma-separated value (CSV) format.
 * The witness IDs are assumed not to contain commas; if they do, then they will need to be manually escaped in the output.
 */
void enumerate_relationships_table::to_csv(ostream & out, const set<string> & filter_relationship_types) {
	//Print the caption:
	out << "Genealogical relationships between " << primary_wit_id << " and " << secondary_wit_id << "\n";
	//Then print the lists of passages for the filtered relationship types:
	if (filter_relationship_types.find("extant") != filter_relationship_types.end()) {
		out << "EXTANT" << "\n";
		for (string vu_id : extant) {
			out << vu_id << "\n";
		}
	}
	if (filter_relationship_types.find("agree") != filter_relationship_types.end()) {
		out << "AGREE" << "\n";
		for (string vu_id : agreements) {
			out << vu_id << "\n";
		}
	}
	if (filter_relationship_types.find("prior") != filter_relationship_types.end()) {
		out << "PRIOR" << "\n";
		for (string vu_id : prior) {
			out << vu_id << "\n";
		}
	}
	if (filter_relationship_types.find("posterior") != filter_relationship_types.end()) {
		out << "POSTERIOR" << "\n";
		for (string vu_id : posterior) {
			out << vu_id << "\n";
		}
	}
	if (filter_relationship_types.find("norel") != filter_relationship_types.end()) {
		out << "NOREL" << "\n";
		for (string vu_id : norel) {
			out << vu_id << "\n";
		}
	}
	if (filter_relationship_types.find("unclear") != filter_relationship_types.end()) {
		out << "UNCLEAR" << "\n";
		for (string vu_id : unclear) {
			out << vu_id << "\n";
		}
	}
	if (filter_relationship_types.find("explained") != filter_relationship_types.end()) {
		out << "EXPLAINED" << "\n";
		for (string vu_id : explained) {
			out << vu_id << "\n";
		}
	}
	out << endl;
	return;
}

/**
 * Given an output stream and a set of desired relationship types, 
 * prints the contents of this table for those relationship types in tab-separated value (TSV) format.
 * The witness IDs are assumed not to contain tabs; if they do, then they will need to be manually escaped in the output.
 */
void enumerate_relationships_table::to_tsv(ostream & out, const set<string> & filter_relationship_types) {
	//Print the caption:
	out << "Genealogical relationships between " << primary_wit_id << " and " << secondary_wit_id << "\n";
	//Then print the lists of passages for the filtered relationship types:
	if (filter_relationship_types.find("extant") != filter_relationship_types.end()) {
		out << "EXTANT" << "\n";
		for (string vu_id : extant) {
			out << vu_id << "\n";
		}
	}
	if (filter_relationship_types.find("agree") != filter_relationship_types.end()) {
		out << "AGREE" << "\n";
		for (string vu_id : agreements) {
			out << vu_id << "\n";
		}
	}
	if (filter_relationship_types.find("prior") != filter_relationship_types.end()) {
		out << "PRIOR" << "\n";
		for (string vu_id : prior) {
			out << vu_id << "\n";
		}
	}
	if (filter_relationship_types.find("posterior") != filter_relationship_types.end()) {
		out << "POSTERIOR" << "\n";
		for (string vu_id : posterior) {
			out << vu_id << "\n";
		}
	}
	if (filter_relationship_types.find("norel") != filter_relationship_types.end()) {
		out << "NOREL" << "\n";
		for (string vu_id : norel) {
			out << vu_id << "\n";
		}
	}
	if (filter_relationship_types.find("unclear") != filter_relationship_types.end()) {
		out << "UNCLEAR" << "\n";
		for (string vu_id : unclear) {
			out << vu_id << "\n";
		}
	}
	if (filter_relationship_types.find("explained") != filter_relationship_types.end()) {
		out << "EXPLAINED" << "\n";
		for (string vu_id : explained) {
			out << vu_id << "\n";
		}
	}
	out << endl;
	return;
}

/**
 * Given an output stream and a set of desired relationship types, 
 * prints the contents of this table for those relationship types in JavaScript Object Notation (JSON) format.
 * The witness IDs are assumed not to contain characters that need to be escaped in URLs.
 */
void enumerate_relationships_table::to_json(ostream & out, const set<string> & filter_relationship_types) {
    //Open the root object:
    out << "{";
    //Add the metadata fields:
	out << "\"primary_wit\":" << "\"" << primary_wit_id << "\"" << ",";
	out << "\"secondary_wit\":" << "\"" << secondary_wit_id << "\"" << ",";
    //Then print the lists of passages for the filtered relationship types:
	unsigned int relationship_types_processed = 0;
	if (filter_relationship_types.find("extant") != filter_relationship_types.end()) {
		out << "\"extant\":" << "[";
		for (string vu_id : extant) {
			out << "\"" << vu_id << "\"" << "\n";
		}
    	out << "]";
		//Increment the number of relationship types processed so far:
		relationship_types_processed++;
		//If this is less than the number of desired relationship types, then add a comma:
		if (relationship_types_processed < filter_relationship_types.size()) {
			out << ",";
		}
	}
	if (filter_relationship_types.find("agree") != filter_relationship_types.end()) {
		out << "\"agree\":" << "[";
		for (string vu_id : agreements) {
			out << "\"" << vu_id << "\"" << "\n";
		}
		out << "]";
		//Increment the number of relationship types processed so far:
		relationship_types_processed++;
		//If this is less than the number of desired relationship types, then add a comma:
		if (relationship_types_processed < filter_relationship_types.size()) {
			out << ",";
		}
	}
	if (filter_relationship_types.find("prior") != filter_relationship_types.end()) {
		out << "\"prior\":" << "[";
		for (string vu_id : prior) {
			out << "\"" << vu_id << "\"" << "\n";
		}
		out << "]";
		//Increment the number of relationship types processed so far:
		relationship_types_processed++;
		//If this is less than the number of desired relationship types, then add a comma:
		if (relationship_types_processed < filter_relationship_types.size()) {
			out << ",";
		}
	}
	if (filter_relationship_types.find("posterior") != filter_relationship_types.end()) {
		out << "\"posterior\":" << "[";
		for (string vu_id : posterior) {
			out << "\"" << vu_id << "\"" << "\n";
		}
		out << "]";
		//Increment the number of relationship types processed so far:
		relationship_types_processed++;
		//If this is less than the number of desired relationship types, then add a comma:
		if (relationship_types_processed < filter_relationship_types.size()) {
			out << ",";
		}
	}
	if (filter_relationship_types.find("norel") != filter_relationship_types.end()) {
		out << "\"norel\":" << "[";
		for (string vu_id : norel) {
			out << "\"" << vu_id << "\"" << "\n";
		}
		out << "]";
		//Increment the number of relationship types processed so far:
		relationship_types_processed++;
		//If this is less than the number of desired relationship types, then add a comma:
		if (relationship_types_processed < filter_relationship_types.size()) {
			out << ",";
		}
	}
	if (filter_relationship_types.find("unclear") != filter_relationship_types.end()) {
		out << "\"unclear\":" << "[";
		for (string vu_id : unclear) {
			out << vu_id << "\n";
		}
		out << "]";
		//Increment the number of relationship types processed so far:
		relationship_types_processed++;
		//If this is less than the number of desired relationship types, then add a comma:
		if (relationship_types_processed < filter_relationship_types.size()) {
			out << ",";
		}
	}
	if (filter_relationship_types.find("explained") != filter_relationship_types.end()) {
		out << "\"explained\":" << "[";
		for (string vu_id : explained) {
			out << vu_id << "\n";
		}
		out << "]";
		//Increment the number of relationship types processed so far:
		relationship_types_processed++;
		//If this is less than the number of desired relationship types, then add a comma:
		if (relationship_types_processed < filter_relationship_types.size()) {
			out << ",";
		}
	}
    //Close the root object:
    out << "}";
	return;
}