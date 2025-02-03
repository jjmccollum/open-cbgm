/*
 * witness.h
 *
 *  Created on: Oct 24, 2019
 *      Author: jjmccollum
 */

#ifndef WITNESS_H
#define WITNESS_H

#include <string>
#include <list>
#include <unordered_map>

#include "roaring.hh"
#include "apparatus.h"
#include "set_cover_solver.h"

 /**
 * Data structure representing a set of genealogical comparisons to a secondary witness relative to a primary witness.
 */
struct genealogical_comparison {
	std::string primary_wit; //ID of the primary witness
	std::string secondary_wit; //ID of the secondary witness
	roaring::Roaring extant; //passages where both witnesses are extant
	roaring::Roaring agreements; //passages where both witnesses agree
	roaring::Roaring prior; //passages where the primary witness has a prior reading
	roaring::Roaring posterior; //passages where the primary witness has a posterior reading
	roaring::Roaring norel; //passages where both witnesses' readings are known to have no directed relationship
	roaring::Roaring unclear; //passages where both witnesses' readings have an unknown relationship
	roaring::Roaring explained; //passages where the primary witness has a reading explained by that of the secondary witness
	float cost; //genealogical cost of relationship
};

class witness {
private:
	std::string id;
	std::unordered_map<std::string, genealogical_comparison> genealogical_comparisons;
	std::list<std::string> potential_ancestor_ids;
	std::list<std::string> stemmatic_ancestor_ids;
public:
	witness();
	witness(const std::string & _id, const apparatus & app, bool classic=false);
	witness(const std::string & _id, const std::list<genealogical_comparison> & _genealogical_comparisons);
	virtual ~witness();
	std::string get_id() const;
	std::unordered_map<std::string, genealogical_comparison> get_genealogical_comparisons() const;
	genealogical_comparison get_genealogical_comparison_for_witness(const std::string & other_id) const;
	std::list<std::string> get_potential_ancestor_ids() const;
	std::list<set_cover_solution> get_substemmata(float ub=0, bool single_solution=false) const;
	void set_stemmatic_ancestor_ids(const std::list<std::string> & witnesses);
	std::list<std::string> get_stemmatic_ancestor_ids() const;
};

#endif /* WITNESS_H */
