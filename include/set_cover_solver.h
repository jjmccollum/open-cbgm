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
#include <vector>
#include <stack>

#include "roaring.hh"

using namespace std;

/**
 * Enumeration of states for an accept-reject branch and bound node.
 */
enum node_state {ACCEPT, REJECT, DONE};

/**
 * Data structure representing a branch and bound node, including its processing state.
 */
struct branch_and_bound_node {
	unsigned int candidate_row;
	node_state state;
};

/**
 * Data structure representing a set cover row, including the ID of the witness it represents and its cost.
 */
struct set_cover_row {
	string id;
	Roaring bits;
	int cost;
};

/**
 * Data structure representing a set cover solution.
 */
struct set_cover_solution {
	list<set_cover_row> rows;
	int cost;
};

class set_cover_solver {
private:
	vector<set_cover_row> rows;
	Roaring target;
	Roaring accepted;
	Roaring remaining;
	stack<branch_and_bound_node> nodes;
	int fixed_ub = -1;
public:
	set_cover_solver();
	set_cover_solver(const vector<set_cover_row> & _rows, const Roaring & _target);
	set_cover_solver(const vector<set_cover_row> & _rows, const Roaring & _target, int _fixed_ub);
	virtual ~set_cover_solver();
	Roaring get_uncovered_columns() const;
	list<set_cover_row> get_unique_rows() const;
	void branch();
	int bound() const;
	bool is_feasible() const;
	bool is_any_branch_feasible() const;
	set_cover_solution get_trivial_solution() const;
	set_cover_solution get_greedy_solution() const;
	void branch_and_bound(list<set_cover_solution> & solutions);
	void solve(list<set_cover_solution> & solutions);
};

#endif /* SET_COVER_SOLVER_H */
