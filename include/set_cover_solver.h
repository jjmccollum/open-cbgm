/*
 * set_cover_solver.h
 *
 *  Created on: Dec 12, 2019
 *      Author: jjmccollum
 */
#ifndef SET_COVER_SOLVER_H
#define SET_COVER_SOLVER_H

#include <string>
#include <list>
#include <stack>
#include <vector>
#include <limits>

#include "roaring.hh"

/**
 * Enumeration of states for an accept-reject branch and bound node.
 */
enum node_state {ACCEPT, REJECT, DONE};

/**
 * Data structure representing a branch and bound node, including its processing state.
 */
struct branch_and_bound_node {
	unsigned int row;
	node_state state;
};

/**
 * Data structure representing a set cover row, including the ID of the witness it represents and its cost.
 */
struct set_cover_row {
	std::string id;
	roaring::Roaring agreements;
	roaring::Roaring explained;
	float cost;
};

/**
 * Data structure representing a set cover solution.
 */
struct set_cover_solution {
	std::list<set_cover_row> rows;
	int agreements;
	float cost;
};

class set_cover_solver {
private:
	std::vector<set_cover_row> rows;
	roaring::Roaring target;
	float fixed_ub = std::numeric_limits<float>::infinity();
public:
	set_cover_solver();
	set_cover_solver(const std::vector<set_cover_row> & _rows, const roaring::Roaring & _target);
	set_cover_solver(const std::vector<set_cover_row> & _rows, const roaring::Roaring & _target, float _fixed_ub);
	virtual ~set_cover_solver();
	set_cover_solution get_solution_from_rows(const roaring::Roaring & solution_rows) const;
	roaring::Roaring get_uncovered_columns() const;
	roaring::Roaring get_unique_rows() const;
	bool is_feasible(const roaring::Roaring & solution_rows) const;
	void remove_redundant_rows_from_solution(roaring::Roaring & initial_solution_rows) const;
	roaring::Roaring get_greedy_solution() const;
	void branch(const roaring::Roaring & remaining, std::stack<branch_and_bound_node> & nodes);
	float bound(const roaring::Roaring & solution_rows) const;
	void branch_and_bound(std::list<set_cover_solution> & solutions);
	void branch_and_bound_single_solution(std::list<set_cover_solution> & solutions);
	void solve(std::list<set_cover_solution> & solutions, bool single_solution=false);
};

#endif /* SET_COVER_SOLVER_H */
