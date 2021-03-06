/*
 * compare_witnesses_table.h
 *
 *  Created on: Sept 17, 2020
 *      Author: jjmccollum
 */

#ifndef COMPARE_WITNESSES_TABLE_H
#define COMPARE_WITNESSES_TABLE_H

#include <iostream>
#include <string>
#include <list>
#include <set>

#include "witness.h"

 /**
 * Data structure representing a row of the witness comparison table.
 */
struct compare_witnesses_table_row {
	std::string id; //ID of the secondary witness
	int dir; //-1 if primary witness is prior; 1 if posterior; 0 otherwise
	int nr; //rank of the secondary witness as a potential ancestor of the primary witness
	int pass; //number of passages where both witnesses are extant
	float perc; //percentage of agreement in passages where both witnesses are extant
	int eq; //number of passages where both witnesses agree
	int prior; //number of passages where the primary witness has a prior reading
	int posterior; //number of passages where the primary witness has a posterior reading
	int norel; //number of passages where both witnesses' readings are known to have no directed relationship
    int uncl; //number of passages where the witnesses' readings have an unclear relationship
    int expl; //number of passages where the primary witness has a reading explained by that of the secondary witness
	float cost; //genealogical cost of relationship from the target witnesses if it is a potential ancestor
};

class compare_witnesses_table {
private:
    std::string id;
    int primary_extant;
	std::list<compare_witnesses_table_row> rows;
public:
	compare_witnesses_table();
	compare_witnesses_table(const witness & wit, const std::list<std::string> list_wit, const std::set<std::string> & filter_wits);
	virtual ~compare_witnesses_table();
    std::string get_id() const;
    int get_primary_extant() const;
    std::list<compare_witnesses_table_row> get_rows() const;
    void to_fixed_width(std::ostream & out);
	void to_csv(std::ostream & out);
    void to_tsv(std::ostream & out);
    void to_json(std::ostream & out);
};

#endif /* COMPARE_WITNESSES_TABLE_H */