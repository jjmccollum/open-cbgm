/*
 * find_relatives_table.cpp
 *
 *  Created on: Sept 18, 2020
 *      Author: jjmccollum
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <list>
#include <set>
#include <algorithm>
#include <limits>

#include "variation_unit.h"
#include "witness.h"
#include "find_relatives_table.h"

 /**
 * Default constructor.
 */
find_relatives_table::find_relatives_table() {

}

/**
 * Constructs a find relatives table relative to a given witness at a given variation unit,
 * given a list of witness IDs in input order and a set of readings by which to filter the rows.
 */
 find_relatives_table::find_relatives_table(const witness & wit, const variation_unit & vu, const list<string> list_wit, const set<string> & filter_rdgs) {
    rows = list<find_relatives_table_row>();
    id = wit.get_id();
	unordered_map<string, string> reading_support = vu.get_reading_support();
    //Start by populating the table completely with this witness's comparisons to all other witnesses:
    for (string secondary_wit_id : list_wit) {
        //Get the genealogical comparison of the primary witness to this witness:
        genealogical_comparison comp = wit.get_genealogical_comparison_for_witness(secondary_wit_id);
        //For the primary witness, copy the number of passages where it is extant and move on:
        if (secondary_wit_id == id) {
            primary_extant = (int) comp.extant.cardinality();
            continue;
        }
        find_relatives_table_row row;
        row.id = secondary_wit_id;
		row.rdg = reading_support.find(row.id) != reading_support.end() ? reading_support.at(row.id) : "-";
        row.pass = (int) comp.extant.cardinality();
        row.eq = (int) comp.agreements.cardinality();
        row.perc = row.pass > 0 ? (100 * float(row.eq) / float(row.pass)) : 0;
        row.prior = (int) comp.prior.cardinality();
        row.posterior = (int) comp.posterior.cardinality();
        row.norel = (int) comp.norel.cardinality();
        row.uncl = (int) comp.unclear.cardinality();
        row.expl = (int) comp.explained.cardinality();
        row.cost = row.prior >= row.posterior ? -1 : comp.cost;
        rows.push_back(row);
    }
    //Sort the list of rows from highest number of agreements to lowest:
    rows.sort([](const find_relatives_table_row & r1, const find_relatives_table_row & r2) {
        return r1.eq > r2.eq;
    });
    //Pass through the sorted list of comparisons to assign relationship directions and ancestral ranks:
    int nr = 0;
    int nr_value = numeric_limits<int>::max();
    for (find_relatives_table_row & row : rows) {
        //Only assign positive ancestral ranks to witnesses prior to this one:
        if (row.posterior > row.prior) {
            //Only increment the rank if the number of agreements is lower than that of the previous potential ancestor:
            if (row.eq < nr_value) {
                nr_value = row.eq;
                nr++;
            }
            row.dir = 1;
            row.nr = nr;
        }
        else if (row.posterior == row.prior) {
            row.dir = 0;
            row.nr = 0;
        }
        else {
            row.dir = -1;
            row.nr = -1;
        }
    }
	//If the filter set is not empty, then filter the rows:
	if (!filter_rdgs.empty()) {
		rows.remove_if([&](const find_relatives_table_row & row) {
			return filter_rdgs.find(row.rdg) == filter_rdgs.end();
		});
	}
}

/**
 * Default destructor.
 */
find_relatives_table::~find_relatives_table() {

}

/**
 * Returns the ID of the primary witness for which this table provides comparisons.
 */
string find_relatives_table::get_id() const {
    return id;
}

/**
 * Returns the number of passages at which this table's primary witness is extant.
 */
int find_relatives_table::get_primary_extant() const {
    return primary_extant;
}

/**
 * Returns this table's list of rows.
 */
list<find_relatives_table_row> find_relatives_table::get_rows() const {
    return rows;
}

/**
 * Given an output stream, prints this find relatives table in fixed-width format.
 */
void find_relatives_table::to_fixed_width(ostream & out) {
    //Print the caption:
	out << "Genealogical comparisons for W1 = " << id << " (" << primary_extant << " extant passages):";
	out << "\n\n";
	//Print the header row:
	out << std::left << std::setw(8) << "W2";
	out << std::left << std::setw(4) << "DIR";
	out << std::right << std::setw(4) << "NR";
	out << std::setw(4) << ""; //buffer space between right-aligned and left-aligned columns
	out << std::left << std::setw(8) << "RDG";
	out << std::right << std::setw(8) << "PASS";
	out << std::right << std::setw(8) << "EQ";
	out << std::right << std::setw(12) << ""; //percentage of agreements among mutually extant passages
	out << std::right << std::setw(8) << "W1>W2";
	out << std::right << std::setw(8) << "W1<W2";
	out << std::right << std::setw(8) << "NOREL";
    out << std::right << std::setw(8) << "UNCL";
    out << std::right << std::setw(8) << "EXPL";
	out << std::right << std::setw(12) << "COST";
	out << "\n\n";
	//Print the subsequent rows:
	for (find_relatives_table_row row : rows) {
		out << std::left << std::setw(8) << row.id;
		out << std::left << std::setw(4) << (row.dir == -1 ? "<" : (row.dir == 1 ? ">" : "="));
		out << std::right << std::setw(4) << (row.nr > 0 ? to_string(row.nr) : "");
		out << std::setw(4) << ""; //buffer space between right-aligned and left-aligned columns
		out << std::left << std::setw(8) << row.rdg;
		out << std::right << std::setw(8) << row.pass;
		out << std::right << std::setw(8) << row.eq;
		out << std::right << std::setw(3) << "(" << std::setw(7) << fixed << std::setprecision(3) << row.perc << std::setw(2) << "%)";
		out << std::right << std::setw(8) << row.prior;
		out << std::right << std::setw(8) << row.posterior;
		out << std::right << std::setw(8) << row.norel;
        out << std::right << std::setw(8) << row.uncl;
        out << std::right << std::setw(8) << row.expl;
		if (row.cost >= 0) {
			out << std::right << std::setw(12) << fixed << std::setprecision(3) << row.cost;
		}
		else {
			out << std::right << std::setw(12) << "";
		}
		out << "\n";
	}
	out << endl;
	return;
}

/**
 * Given an output stream, prints this find relatives table in comma-separated value (CSV) format.
 * The witness IDs are assumed not to contain commas; if they do, then they will need to be manually escaped in the output.
 */
void find_relatives_table::to_csv(ostream & out) {
    //Print the header row:
	out << "W2" << ",";
	out << "DIR" << ",";
	out << "NR" << ",";
	out << "RDG" << ",";
	out << "PASS" << ",";
	out << "EQ" << ",";
	out << "" << ","; //percentage of agreements among mutually extant passages
	out << "W1>W2" << ",";
	out << "W1<W2" << ",";
	out << "NOREL" << ",";
    out << "UNCL" << ",";
    out << "EXPL" << ",";
	out << "COST" << "\n";
	//Print the subsequent rows:
	for (find_relatives_table_row row : rows) {
		out << row.id << ",";
		out << (row.dir == -1 ? "<" : (row.dir == 1 ? ">" : "=")) << ",";
		out << (row.nr > 0 ? to_string(row.nr) : "") << ",";
		out << row.rdg << ",";
		out << row.pass << ",";
		out << row.eq << ",";
		out << "(" << row.perc << "%)" << ",";
		out << row.prior << ",";
		out << row.posterior << ",";
		out << row.norel << ",";
        out << row.uncl << ",";
        out << row.expl << ",";
		if (row.cost >= 0) {
			out << row.cost << "\n";
		}
		else {
			out << "" << "\n";
		}
	}
	out << endl;
	return;
}

/**
 * Given an output stream, prints this find relatives table in tab-separated value (TSV) format.
 * The witness IDs are assumed not to contain tabs; if they do, then they will need to be manually escaped in the output.
 */
void find_relatives_table::to_tsv(ostream & out) {
    //Print the header row:
	out << "W2" << "\t";
	out << "DIR" << "\t";
	out << "NR" << "\t";
	out << "RDG" << "\t";
	out << "PASS" << "\t";
	out << "EQ" << "\t";
	out << "" << "\t"; //percentage of agreements among mutually extant passages
	out << "W1>W2" << "\t";
	out << "W1<W2" << "\t";
	out << "NOREL" << "\t";
    out << "UNCL" << "\t";
    out << "EXPL" << "\t";
	out << "COST" << "\n";
	//Print the subsequent rows:
	for (find_relatives_table_row row : rows) {
		out << row.id << "\t";
		out << (row.dir == -1 ? "<" : (row.dir == 1 ? ">" : "=")) << "\t";
		out << (row.nr > 0 ? to_string(row.nr) : "") << "\t";
		out << row.rdg << "\t";
		out << row.pass << "\t";
		out << row.eq << "\t";
		out << "(" << row.perc << "%)" << "\t";
		out << row.prior << "\t";
		out << row.posterior << "\t";
		out << row.norel << "\t";
        out << row.uncl << "\t";
        out << row.expl << "\t";
		if (row.cost >= 0) {
			out << row.cost << "\n";
		}
		else {
			out << "" << "\n";
		}
	}
	out << endl;
	return;
}

/**
 * Given an output stream, prints this find relatives table in JavaScript Object Notation (JSON) format.
 * The witness IDs are assumed not to contain tabs; if they do, then they will need to be manually escaped in the output.
 */
void find_relatives_table::to_json(ostream & out) {
    //Open the root object:
    out << "{";
    //Add the metadata fields:
	out << "\"W1\":" << "\"" << id << "\"" << ",";
    out << "\"W1_PASS\":" << primary_extant << ",";
    //Open the rows array:
    out << "\"rows\":" << "[";
    //Print each row as an object:
	unsigned int row_num = 0;
	for (find_relatives_table_row row : rows) {
        //Open the row object:
        out << "{";
        //Add its key-value pairs:
        out << "\"W2\":" << "\"" << row.id << "\"" << ",";
		out << "\"DIR\":" << "\"" << (row.dir == -1 ? "<" : (row.dir == 1 ? ">" : "=")) << "\"" << ",";
		out << "\"NR\":" << "\"" << (row.nr > 0 ? to_string(row.nr) : "") << "\"" << ",";
		out << "\"RDG\":" << row.rdg << ",";
		out << "\"PASS\":" << row.pass << ",";
		out << "\"EQ\":" << row.eq << ",";
        out << "\"PERC\":" << row.perc << ",";
		out << "\"W1>W2\":" << row.prior << ",";
		out << "\"W1<W2\":" << row.posterior << ",";
		out << "\"NOREL\":" << row.norel << ",";
        out << "\"UNCL\":" << row.uncl << ",";
        out << "\"EXPL\":" << row.expl << ",";
		if (row.cost >= 0) {
			out << "\"COST\":" << "\"" << row.cost << "\"";
		}
		else {
			out << "\"COST\":" << "\"" << "" << "\"";
		}
        //Close the row object:
        out << "}";
        //Add a comma if this is not the last row:
        if (row_num < rows.size() - 1) {
            out << ",";
        }
		row_num++;
	}
    //Close the rows array:
    out << "]";
    //Close the root object, and flush the stream:
    out << "}" << endl;
	return;
}