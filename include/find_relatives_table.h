/*
 * find_relatives_table.h
 *
 *  Created on: Sept 18, 2020
 *      Author: jjmccollum
 */

#ifndef FIND_RELATIVES_TABLE_H
#define FIND_RELATIVES_TABLE_H

#include <iostream>
#include <string>
#include <list>
#include <set>

#include "variation_unit.h"
#include "witness.h"

 /**
 * Data structure representing a row of the witness comparison table.
 */
struct find_relatives_table_row {
	std::string id; //ID of the secondary witness
	int dir; //-1 if primary witness is prior; 1 if posterior; 0 otherwise
	int nr; //rank of the secondary witness as a potential ancestor of the primary witness
	std::string rdg; //reading of the secondary witness at the given passage
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

class find_relatives_table {
private:
    std::string id;
    std::string label;
    int connectivity;
    int primary_extant;
    std::string primary_rdg;
	std::list<find_relatives_table_row> rows;
public:
	find_relatives_table();
	find_relatives_table(const witness & wit, const variation_unit & vu, const std::list<std::string> list_wit, const std::set<std::string> & filter_wits);
	virtual ~find_relatives_table();
    std::string get_id() const;
    std::string get_label() const;
    int get_connectivity() const;
    int get_primary_extant() const;
    std::string get_primary_rdg() const;
    std::list<find_relatives_table_row> get_rows() const;
    void to_fixed_width(std::ostream & out);
	void to_csv(std::ostream & out);
    void to_tsv(std::ostream & out);
    void to_json(std::ostream & out);
};

#endif /* FIND_RELATIVES_TABLE_H */
