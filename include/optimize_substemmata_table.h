/*
 * optimize_substemmata_table.h
 *
 *  Created on: Sept 18, 2020
 *      Author: jjmccollum
 */

#ifndef OPTIMIZE_SUBSTEMMATA_TABLE_H
#define OPTIMIZE_SUBSTEMMATA_TABLE_H

#include <iostream>
#include <string>
#include <list>
#include <set>

#include "set_cover_solver.h"
#include "witness.h"

using namespace std;

/**
 * Data structure representing a row of the optimize substemmata table.
 */
struct optimize_substemmata_table_row {
	list<string> ancestors; //list of stemmatic ancestor IDs
	float cost; //total cost of substemma
	int agreements; //number of passages in primary witness explained by agreement with at least one stemmatic ancestor
};

class optimize_substemmata_table {
private:
    string id;
    int primary_extant;
	list<optimize_substemmata_table_row> rows;
public:
	optimize_substemmata_table();
	optimize_substemmata_table(const witness & wit, float ub);
	virtual ~optimize_substemmata_table();
    string get_id() const;
    int get_primary_extant() const;
    list<optimize_substemmata_table_row> get_rows() const;
    void to_fixed_width(ostream & out);
	void to_csv(ostream & out);
    void to_tsv(ostream & out);
    void to_json(ostream & out);
};

#endif /* OPTIMIZE_SUBSTEMMATA_TABLE_H */
