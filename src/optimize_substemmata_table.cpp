/*
 * optimize_substemmata_table.cpp
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

#include "roaring.hh"
#include "set_cover_solver.h"
#include "witness.h"
#include "optimize_substemmata_table.h"

 /**
 * Default constructor.
 */
optimize_substemmata_table::optimize_substemmata_table() {

}

/**
 * Constructs an optimize substemmata table for a given witness (assumed to have its potential ancestors list populated).
 * Optionally, a fixed upper bound on substemma costs can be specified, in which case the table will enumerate all substemmata with costs within this bound.
 * If no bound is specified, then only the substemmata with the lowest cost will be enumerated.
 */
 optimize_substemmata_table::optimize_substemmata_table(const witness & wit, float ub=0) {
    id = wit.get_id();
	rows = wit.get_substemmata(ub);
}

/**
 * Default destructor.
 */
optimize_substemmata_table::~optimize_substemmata_table() {

}

/**
 * Returns the ID of the primary witness for which this table provides comparisons.
 */
string optimize_substemmata_table::get_id() const {
    return id;
}

/**
 * Returns this table's list of rows.
 */
list<set_cover_solution> optimize_substemmata_table::get_rows() const {
    return rows;
}

/**
 * Given an output stream, prints this substemma optimization table in fixed-width format.
 */
void optimize_substemmata_table::to_fixed_width(ostream & out) {
    //Print the caption:
	out << "Optimal substemmata for witness W1 = " << id << ":\n\n";
	//Print the header row:
	out << std::left << std::setw(48) << "SUBSTEMMA";
	out << std::right << std::setw(8) << "COST";
	out << std::right << std::setw(8) << "AGREE";
	out << "\n\n";
	//Print the subsequent rows:
	for (set_cover_solution row : rows) {
		string substemma_str = "";
		for (set_cover_row sc_row : row.rows) {
			substemma_str += sc_row.id;
			if (sc_row.id != row.rows.back().id) {
				substemma_str += ", ";
			}
		}
		out << std::left << std::setw(48) << substemma_str;
		out << std::right << std::setw(8) << row.cost;
		out << std::right << std::setw(8) << row.agreements;
		out << "\n";
	}
	out << endl;
	return;
}

/**
 * Given an output stream, prints this substemma optimization table in comma-separated value (CSV) format.
 * The witness IDs are assumed not to contain commas; if they do, then they will need to be manually escaped in the output.
 */
void optimize_substemmata_table::to_csv(ostream & out) {
    //Print the header row:
	out << "SUBSTEMMA" << ",";
	out << "COST" << ",";
	out << "AGREE" << "\n";
	//Print the subsequent rows:
	for (set_cover_solution row : rows) {
		string substemma_str = "";
		substemma_str += "\""; //place in quotes to escape commas
		for (set_cover_row sc_row : row.rows) {
			substemma_str += sc_row.id;
			if (sc_row.id != row.rows.back().id) {
				substemma_str += ", ";
			}
		}
		substemma_str += "\"";
		out << substemma_str << ",";
		out << row.cost << ",";
		out << row.agreements << "\n";
	}
	out << endl;
	return;
}

/**
 * Given an output stream prints this substemma optimization table in tab-separated value (TSV) format.
 * The witness IDs are assumed not to contain tabs; if they do, then they will need to be manually escaped in the output.
 */
void optimize_substemmata_table::to_tsv(ostream & out) {
    //Print the header row:
	out << "SUBSTEMMA" << "\t";
	out << "COST" << "\t";
	out << "AGREE" << "\n";
	//Print the subsequent rows:
	for (set_cover_solution row : rows) {
		string substemma_str = "";
		for (set_cover_row sc_row : row.rows) {
			substemma_str += sc_row.id;
			if (sc_row.id != row.rows.back().id) {
				substemma_str += ", ";
			}
		}
		out << substemma_str << "\t";
		out << row.cost << "\t";
		out << row.agreements << "\n";
	}
	out << endl;
	return;
}

/**
 * Given an output stream, prints this substemma optimization table in JavaScript Object Notation (JSON) format.
 * The witness IDs are assumed not to contain tabs; if they do, then they will need to be manually escaped in the output.
 */
void optimize_substemmata_table::to_json(ostream & out) {
    //Open the root object:
    out << "{";
    //Add the metadata fields:
	out << "\"W1\":" << "\"" << id << "\"" << ",";
    //Open the rows array:
    out << "\"rows\":" << "[";
    //Print each row as an object:
	unsigned int row_num = 0;
	for (set_cover_solution row : rows) {
        //Open the row object:
        out << "{";
        //Add its key-value pairs:
        out << "\"SUBSTEMMA\":";
		//Open the substemma array:
		out << "[";
		for (set_cover_row sc_row : row.rows) {
			out << "\"" << sc_row.id << "\"";
			if (sc_row.id != row.rows.back().id) {
				out << ",";
			}
		}
		//Close the substemma array:
		out << "]";
		out << ",";
		out << "\"COST\":" << row.cost << ",";
		out << "\"AGREE\":" << row.agreements;
        //Close the row object:
        out << "}";
        //Add a comma if this is not the last row:
        if (row_num != rows.size() - 1) {
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