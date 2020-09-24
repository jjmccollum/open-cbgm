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

class optimize_substemmata_table {
private:
    string id;
	list<set_cover_solution> rows;
public:
	optimize_substemmata_table();
	optimize_substemmata_table(const witness & wit, float ub);
	virtual ~optimize_substemmata_table();
    string get_id() const;
    list<set_cover_solution> get_rows() const;
    void to_fixed_width(ostream & out);
	void to_csv(ostream & out);
    void to_tsv(ostream & out);
    void to_json(ostream & out);
};

#endif /* OPTIMIZE_SUBSTEMMATA_TABLE_H */