/*
 * set_cover_solver.cpp
 *
 *  Created on: Dec 12, 2019
 *      Author: jjmccollum
 */

#include <string>
#include <list>
#include <vector>
#include <stack>

#include "set_cover_solver.h"
#include "roaring.hh"

using namespace std;

/**
 * Default constructor.
 */
set_cover_solver::set_cover_solver() {

}

/**
 * Constructs a set cover solver given a vector of row data structures (assumed to be sorted by ascending costs)
 * and a bitmap representing the target set to cover.
 */
set_cover_solver::set_cover_solver(const vector<set_cover_row> & _rows, const Roaring & _target) {
	//Copy the input rows and target set:
	rows = vector<set_cover_row>(_rows);
	target = Roaring(_target);
	//Initialize bitmaps representing rows included in the current solution and rows to be processed:
	accepted = Roaring();
	remaining = Roaring();
	remaining.addRange(0, rows.size());
	//Initialize a stack of branch-and-bound nodes:
	nodes = stack<branch_and_bound_node>();
}

/**
 * Constructs a set cover solver given a vector of row data structures (assumed to be sorted by ascending costs),
 * a bitmap representing the target set to cover, and a fixed upper bound.
 */
set_cover_solver::set_cover_solver(const vector<set_cover_row> & _rows, const Roaring & _target, int _fixed_ub) {
	//Copy the input rows and target set:
	rows = vector<set_cover_row>(_rows);
	target = Roaring(_target);
	//Initialize bitmaps representing rows included in the current solution and rows to be processed:
	accepted = Roaring();
	remaining = Roaring();
	remaining.addRange(0, rows.size());
	//Initialize a stack of branch-and-bound nodes:
	nodes = stack<branch_and_bound_node>();
	//Set the fixed upper bound:
	fixed_ub = _fixed_ub;
}

/**
 * Default destructor.
 */
set_cover_solver::~set_cover_solver() {

}

/**
 * Returns a bitmap of target columns not covered by any row.
 */
Roaring set_cover_solver::get_uncovered_columns() const {
	//Get the union of all rows:
	Roaring row_union = Roaring();
	for (set_cover_row row : rows) {
		row_union |= row.bits;
	}
	return target ^ (target & row_union);
}

/**
 * Returns a list of set cover rows that uniquely cover one or more columns
 * (and therefore must be included in any solution).
 */
list<set_cover_row> set_cover_solver::get_unique_rows() const {
	list<set_cover_row> unique_rows = list<set_cover_row>();
	Roaring unique_row_inds = Roaring();
	//If there are no set cover rows, then return an empty list:
	if (rows.empty()) {
		return unique_rows;
	}
	//Otherwise, we will construct a union tree from the bitmaps representing the rows;
	//the tree will consist of 2n - 1 nodes, where n is the number of set cover rows:
	unsigned int n = rows.size();
	vector<Roaring> union_tree = vector<Roaring>(2*n - 1);
	//The last n nodes will represent the set cover rows directly:
	for (int i = n - 1; i >= 0; i--) {
		set_cover_row row = rows[i];
		union_tree[n - 1 + i] = row.bits;
	}
	//The first n - 1  nodes will represent the ancestor nodes:
	for (int i = n - 2; i >= 0; i--) {
		union_tree[i] = union_tree[2*i + 1] | union_tree[2*i + 2];
	}
	//Now proceed for each column in the target set:
	for (Roaring::const_iterator it = target.begin(); it != target.end(); it++) {
		unsigned int col_ind = *it;
		//Now proceed down the union tree to see if this column is uniquely covered:
		unsigned int p = 0;
		while (p < n - 1) {
			Roaring left = union_tree[2*p + 1];
			Roaring right = union_tree[2*p + 2];
			if (left.contains(col_ind) && right.contains(col_ind)) {
				//At least two rows cover this column, so we're done:
				break;
			}
			else if (left.contains(col_ind)) {
				//Recurse on the left child:
				p = 2*p + 1;
			}
			else if (right.contains(col_ind)) {
				//Recurse on the right child:
				p = 2*p + 2;
			}
			else {
				//No rows cover this column (this can only happen at the root of the union tree), so we're done:
				break;
			}
		}
		//If the pointer is at the index of a single row in the union tree, then that row uniquely covers this column:
		if (p >= n - 1) {
			unsigned int row_ind = p - (n - 1);
			unique_row_inds.add(row_ind);
		}
	}
	//Now populate the list of unique coverage rows in order:
	for (Roaring::const_iterator it = unique_row_inds.begin(); it != unique_row_inds.end(); it++) {
		unsigned int row_ind = *it;
		set_cover_row row = rows[row_ind];
		unique_rows.push_back(row);
	}
	return unique_rows;
}

/**
 * Adds the next candidate solution node to the stack.
 */
void set_cover_solver::branch() {
	//Update the target set using the current set of accepted rows:
	Roaring uncovered = Roaring(target);
	for (Roaring::const_iterator it = accepted.begin(); it != accepted.end(); it++) {
		unsigned int row_ind = *it;
		set_cover_row row = rows[row_ind];
		uncovered ^= uncovered & row.bits;
	}
	//Find the first remaining row:
	unsigned int next_row_ind = 0;
	remaining.select(0, & next_row_ind);
	//Add a node for it:
	branch_and_bound_node next_node;
	next_node.candidate_row = next_row_ind;
	next_node.state = node_state::ACCEPT;
	nodes.push(next_node);
	return;
}

/**
 * Returns a lower bound on the cost of any solution that contains the current set of accepted rows.
 */
int set_cover_solver::bound() const {
	int bound = 0;
	for (Roaring::const_iterator it = accepted.begin(); it != accepted.end(); it++) {
		unsigned int row_ind = *it;
		set_cover_row row = rows[row_ind];
		bound += row.cost;
	}
	return bound;
}

/**
 * Returns a boolean value indicating if the current set of accepted rows constitutes a feasible set cover solution.
 */
bool set_cover_solver::is_feasible() const {
	//Check if the target set is covered by the accepted rows:
	Roaring row_union = Roaring();
	for (Roaring::const_iterator it = accepted.begin(); it != accepted.end(); it++) {
		unsigned int row_ind = *it;
		set_cover_row row = rows[row_ind];
		row_union |= row.bits;
		if (target.isSubset(row_union)) {
			return true;
		}
	}
	return false;
}

/**
 * Returns a boolean value indicating if any solution containing the current set of accepted rows is feasible.
 */
bool set_cover_solver::is_any_branch_feasible() const {
	//Check if the target set is covered by the accepted and remaining rows:
	Roaring rows_in_branch = accepted | remaining;
	Roaring row_union = Roaring();
	for (Roaring::const_iterator it = rows_in_branch.begin(); it != rows_in_branch.end(); it++) {
		unsigned int row_ind = *it;
		set_cover_row row = rows[row_ind];
		row_union |= row.bits;
		if (target.isSubset(row_union)) {
			return true;
		}
	}
	return false;
}

/**
 * Returns a trivial set cover solution consisting of the lowest-cost row that covers the target columns.
 * If the current witness has the Ausgangstext as a potential ancestor (which should hold for all non-fragmentary witnesses)
 * and the Ausgangstext explains all other readings
 * (i.e., if all local stemmata are connected, which is necessary for the global stemma to be connected),
 * then at least one such solution is guaranteed to exist.
 */
set_cover_solution set_cover_solver::get_trivial_solution() const {
	set_cover_solution trivial_solution;
	trivial_solution.rows = list<set_cover_row>();
	trivial_solution.cost = target.cardinality() + 1; //this should be larger than any row's cost
	for (set_cover_row row : rows) {
		if (target.isSubset(row.bits) && row.cost < trivial_solution.cost) {
			trivial_solution.rows = list<set_cover_row>({row});
			trivial_solution.cost = row.cost;
		}
	}
	return trivial_solution;
}

/**
 * Returns the set cover solution found by the basic greedy heuristic.
 */
set_cover_solution set_cover_solver::get_greedy_solution() const {
	Roaring initial_greedy_solution_candidates = Roaring();
	Roaring uncovered = Roaring(target);
	//Until the target is completely covered, choose the row with the lowest cost-to-coverage proportion:
	while (uncovered.cardinality() > 0) {
		float best_density = numeric_limits<float>::infinity();
		unsigned int best_row_ind = 0;
		unsigned int row_ind = 0;
		for (set_cover_row row : rows) {
			int cost = row.cost;
			int coverage = (uncovered & row.bits).cardinality();
			//Skip if there is no coverage:
			if (coverage == 0) {
				row_ind++;
				continue;
			}
			float density = float(cost) / float(coverage);
			if (density < best_density && !initial_greedy_solution_candidates.contains(row_ind)) {
				best_density = density;
				best_row_ind = row_ind;
			}
			row_ind++;
		}
		//Add the best-found row to the solution, and remove its overlap with the target set from the target set:
		initial_greedy_solution_candidates.add(best_row_ind);
		set_cover_row best_row = rows[best_row_ind];
		uncovered ^= uncovered & best_row.bits;
	}
	//Now loop backwards through the set of added row indices to remove the highest-cost redundant columns:
	Roaring reduced_greedy_solution_candidates = Roaring(initial_greedy_solution_candidates);
	set_cover_solution reduced_greedy_solution;
	reduced_greedy_solution.cost = 0;
	while (initial_greedy_solution_candidates.cardinality() > 0) {
		//Get the highest-index (i.e., highest-cost) row:
		unsigned int row_ind = 0;
		initial_greedy_solution_candidates.select(initial_greedy_solution_candidates.cardinality() - 1, & row_ind);
		set_cover_row row = rows[row_ind];
		//Check if it is redundant (i.e., if the other rows still in the reduced solution set can cover the original target set):
		reduced_greedy_solution_candidates.remove(row_ind);
		Roaring row_union = Roaring();
		for (Roaring::const_iterator it = reduced_greedy_solution_candidates.begin(); it != reduced_greedy_solution_candidates.end(); it++) {
			unsigned int other_row_ind = *it;
			set_cover_row other_row = rows[other_row_ind];
			row_union |= other_row.bits;
		}
		if (!target.isSubset(row_union)) {
			//If it isn't redundant, then add it back to the set of candidates and include it in the reduced greedy solution:
			reduced_greedy_solution_candidates.add(row_ind);
			reduced_greedy_solution.rows.push_front(row);
			reduced_greedy_solution.cost += row.cost;
		}
		//Pop this row from the back of the initial set:
		initial_greedy_solution_candidates.remove(row_ind);
	}
	return reduced_greedy_solution;
}

/**
 * Populates the given solution list with progressively best-found solutions via branch and bound.
 * If the set cover solver was constructed with a fixed upper bound, then this method will enumerate all solutions with costs within that bound.
 */
void set_cover_solver::branch_and_bound(list<set_cover_solution> & solutions) {
	//If no fixed upper bound is specified, then obtain a good initial upper bound quickly using the trivial solution and the greedy solution:
	int ub = fixed_ub;
	if (fixed_ub < 0) {
		set_cover_solution trivial_solution = get_trivial_solution();
		set_cover_solution greedy_solution = get_greedy_solution();
		ub = min(trivial_solution.cost, greedy_solution.cost);
	}
	//Initialize the stack of branch and bound nodes with the first node:
	branch();
	//Then continue with branch and bound until there is nothing left to be processed:
	while (!nodes.empty()) {
		//Get the current node from the stack:
		branch_and_bound_node & node = nodes.top();
		//Adjust the set partitions to reflect the candidate solution representing by the current node:
		unsigned int candidate_row = node.candidate_row;
		if (node.state == node_state::ACCEPT) {
			//Add the candidate row to the solution:
			remaining.remove(candidate_row);
			accepted.add(candidate_row);
			//Update its state:
			node.state = node_state::REJECT;
		}
		else if (node.state == node_state::REJECT) {
			//Exclude the candidate row from the solution:
			accepted.remove(candidate_row);
			//Update its state:
			node.state = node_state::DONE;
		}
		else {
			//We're done processing this node, and we can add its row back to the set of available rows:
			remaining.add(candidate_row);
			nodes.pop();
			continue;
		}
		//Lower-bound the cost of any solution under the current node:
		int lb = bound();
		//Check if this node represents a feasible solution:
		if (is_feasible()) {
			//If it is, then the lower bound we calculated is this solution's cost; check if it is within the current upper bound:
			if (lb <= ub) {
				//If it is within the upper bound, then add a solution to the list;
				//if the upper bound is not fixed, then update the upper bound, as well:
				if (fixed_ub < 0) {
					ub = lb;
				}
				set_cover_solution solution;
				solution.rows = list<set_cover_row>();
				for (Roaring::const_iterator it = accepted.begin(); it != accepted.end(); it++) {
					unsigned int row_ind = *it;
					set_cover_row row = rows[row_ind];
					solution.rows.push_back(row);
				}
				solution.cost = lb;
				solutions.push_back(solution);
			}
		}
		//If it is not, then check if there is any feasible solution under the current node:
		else if (is_any_branch_feasible()) {
			//If there is, then check if it could have a cost within the upper bound:
			if (lb <= ub) {
				//If so, then branch on this node:
				branch();
			}
		}
	}
	return;
}

/**
 * Populates the given solution list with solutions to the set cover problem.
 * If the set cover solver was constructed with a fixed upper bound, then this method will enumerate all solutions with costs within that bound.
 */
void set_cover_solver::solve(list<set_cover_solution> & solutions) {
	solutions = list<set_cover_solution>();
	//If any column cannot be covered by the rows provided, then we're done:
	if (!is_any_branch_feasible()) {
		return;
	}
	//If any rows uniquely cover one or more columns, then those rows must be set aside to be included in the solution:
	list<set_cover_row> unique_rows = get_unique_rows();
	//Reduce the current problem to an easier subproblem by removing all columns covered by the unique coverage rows from the target set:
	Roaring subproblem_target = Roaring(target);
	int subproblem_fixed_ub = fixed_ub;
	for (set_cover_row row : unique_rows) {
		subproblem_target ^= subproblem_target & row.bits;
		subproblem_fixed_ub -= row.cost;
	}
	//If a fixed upper bound was specified and the total cost of the unique coverage rows exceeds it, then there is no solution:
	if (fixed_ub >= 0 && subproblem_fixed_ub < 0) {
		return;
	}
	//If no columns need to be covered anymore, then we're done:
	if (subproblem_target.cardinality() == 0) {
		//Otherwise, the unique coverage rows constitute the lowest-cost solution:
		set_cover_solution solution;
		solution.cost = 0;
		for (set_cover_row row : unique_rows) {
			solution.rows.push_back(row);
			solution.cost += row.cost;
		}
		solutions.push_back(solution);
		return;
	}
	//Otherwise, solve the subproblem using branch-and-bound:
	vector<set_cover_row> subproblem_rows = vector<set_cover_row>();
	for (set_cover_row row : rows) {
		if ((row.bits & target).cardinality() > 0) {
			subproblem_rows.push_back(row);
		}
	}
	list<set_cover_solution> subproblem_solutions = list<set_cover_solution>();
	set_cover_solver subproblem_solver = fixed_ub >= 0 ? set_cover_solver(subproblem_rows, subproblem_target, subproblem_fixed_ub) : set_cover_solver(subproblem_rows, subproblem_target);
	subproblem_solver.branch_and_bound(subproblem_solutions);
	//Sort the subproblem solutions in order of their costs, then cardinalities:
	subproblem_solutions.sort([](const set_cover_solution & s1, const set_cover_solution & s2) {
		return s1.cost < s2.cost ? true : (s1.cost > s2.cost ? false : (s1.rows.size() < s2.rows.size() ? true : (s1.rows.size() > s2.rows.size() ? false : false)));
	});
	//Then add the unique coverage rows found earlier to the subproblem solutions:
	for (set_cover_solution subproblem_solution : subproblem_solutions) {
		set_cover_solution solution;
		solution.rows = list<set_cover_row>(subproblem_solution.rows);
		solution.cost = subproblem_solution.cost;
		for (set_cover_row row : unique_rows) {
			solution.rows.push_front(row);
			solution.cost += row.cost;
		}
		solutions.push_back(solution);
	}
	return;
}
