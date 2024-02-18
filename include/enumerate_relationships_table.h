/*
 * enumerate_relationships_table.h
 *
 *  Created on: Feb 18, 2024
 *      Author: jjmccollum
 */

#ifndef ENUMERATE_RELATIONSHIPS_TABLE_H
#define ENUMERATE_RELATIONSHIPS_TABLE_H

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <set>

#include "witness.h"

class enumerate_relationships_table {
private:
    std::string primary_wit_id;//ID of the primary witness
	std::string secondary_wit_id; //ID of the secondary witness
	std::list<std::string> extant; //list of passages where both witnesses are extant
	std::list<std::string> agreements; //list of passages where both witnesses agree
	std::list<std::string> prior; //list of passages where the primary witness has a prior reading
	std::list<std::string> posterior; //list of passages where the primary witness has a posterior reading
	std::list<std::string> norel; //list of passages where both witnesses' readings are known to have no directed relationship
    std::list<std::string> unclear; //list of passages where the witnesses' readings have an unclear relationship
    std::list<std::string> explained; //list of passages where the primary witness has a reading explained by that of the secondary witness
public:
	enumerate_relationships_table();
	enumerate_relationships_table(const genealogical_comparison & comp, const std::vector<std::string> & variation_unit_ids);
	virtual ~enumerate_relationships_table();
    std::string get_primary_wit_id() const;
    std::string get_secondary_wit_id() const;
    std::list<std::string> get_extant() const;
    std::list<std::string> get_agreements() const;
    std::list<std::string> get_prior() const;
    std::list<std::string> get_posterior() const;
    std::list<std::string> get_norel() const;
    std::list<std::string> get_unclear() const;
    std::list<std::string> get_explained() const;
    void to_fixed_width(std::ostream & out, const std::set<std::string> & filter_relationship_types);
	void to_csv(std::ostream & out, const std::set<std::string> & filter_relationship_types);
    void to_tsv(std::ostream & out, const std::set<std::string> & filter_relationship_types);
    void to_json(std::ostream & out, const std::set<std::string> & filter_relationship_types);
};

#endif /* ENUMERATE_RELATIONSHIPS_TABLE_H */
