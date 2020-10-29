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

/**
 * Data structure representing a row of the optimize substemmata table.
 */
struct optimize_substemmata_table_row {
	std::list<std::string> ancestors; //list of stemmatic ancestor IDs
	float cost; //total cost of substemma
	int agreements; //number of passages in primary witness explained by agreement with at least one stemmatic ancestor
};

class optimize_substemmata_table {
private:
    std::string id;
    int primary_extant;
	std::list<optimize_substemmata_table_row> rows;
public:
	optimize_substemmata_table();
	optimize_substemmata_table(const witness & wit, float ub);
	virtual ~optimize_substemmata_table();
    std::string get_id() const;
    int get_primary_extant() const;
    std::list<optimize_substemmata_table_row> get_rows() const;
    void to_fixed_width(std::ostream & out);
	void to_csv(std::ostream & out);
    void to_tsv(std::ostream & out);
    void to_json(std::ostream & out);
};

#endif /* OPTIMIZE_SUBSTEMMATA_TABLE_H */
