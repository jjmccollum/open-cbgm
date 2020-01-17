/*
 * set_cover_solver.cpp
 *
 *  Created on: Dec 12, 2019
 *      Author: jjmccollum
 */

#include <string>
#include <list>
#include <stack>
#include <vector>
#include <unordered_map>
#include <limits>

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
}

/**
 * Constructs a set cover solver given a vector of row data structures (assumed to be sorted by ascending costs),
 * a bitmap representing the target set to cover, and a fixed upper bound.
 */
set_cover_solver::set_cover_solver(const vector<set_cover_row> & _rows, const Roaring & _target, float _fixed_ub) {
	//Copy the input rows and target set:
	rows = vector<set_cover_row>(_rows);
	target = Roaring(_target);
	//Set the fixed upper bound:
	fixed_ub = _fixed_ub;
}

/**
 * Default destructor.
 */
set_cover_solver::~set_cover_solver() {

}

/**
 * Given a bitmap representing a set of rows in a solution,
 * returns a set cover solution data structure containing those rows.
 */
set_cover_solution set_cover_solver::get_solution_from_rows(const Roaring & solution_rows) const {
	set_cover_solution solution;
	solution.rows = list<set_cover_row>();
	solution.cost = 0;
	Roaring agreements = Roaring();
	for (Roaring::const_iterator it = solution_rows.begin(); it != solution_rows.end(); it++) {
		unsigned int row_ind = *it;
		set_cover_row row = rows[row_ind];
		solution.rows.push_back(row);
		solution.cost += row.cost;
		agreements |= row.agreements;
	}
	solution.agreements = agreements.cardinality();
	return solution;
}

/**
 * Returns a bitmap of target columns not covered by any row.
 */
Roaring set_cover_solver::get_uncovered_columns() const {
	//Get the union of all rows:
	Roaring row_union = Roaring();
	for (set_cover_row row : rows) {
		row_union |= row.explained;
	}
	return target ^ (target & row_union);
}

/**
 * Returns a bitmap representing rows that uniquely cover one or more columns
 * (and therefore must be included in any solution).
 */
Roaring set_cover_solver::get_unique_rows() const {
	Roaring unique_rows = Roaring();
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
		union_tree[n - 1 + i] = row.explained;
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
			unique_rows.add(row_ind);
		}
	}
	return unique_rows;
}

/**
 * Given a bitmap representing a set of rows,
 * returns a boolean value indicating if that set of rows constitutes a feasible set cover solution.
 */
bool set_cover_solver::is_feasible(const Roaring & solution_rows) const {
	//Check if the target set is covered by the accepted rows:
	Roaring row_union = Roaring();
	for (Roaring::const_iterator it = solution_rows.begin(); it != solution_rows.end(); it++) {
		unsigned int row_ind = *it;
		set_cover_row row = rows[row_ind];
		row_union |= row.explained;
		if (target.isSubset(row_union)) {
			return true;
		}
	}
	return false;
}

/**
 * Given a bitmap representing the rows included in a solution,
 * does a backwards pass through the solution rows and removes any that are not necessary to the solution's feasibility.
 */
void set_cover_solver::remove_redundant_rows_from_solution(Roaring & solution_rows) const {
	//Loop backwards through the set of solution row indices to remove the highest-cost redundant columns:
	Roaring unprocessed_rows = Roaring(solution_rows);
	while (!unprocessed_rows.isEmpty()) {
		//Get the highest-index (i.e., highest-cost) unprocessed row:
		unsigned int row_ind = unprocessed_rows.maximum();
		set_cover_row row = rows[row_ind];
		//Remove the row and check if it is redundant (i.e., if the reduced solution set without it is feasible):
		solution_rows.remove(row_ind);
		if (!is_feasible(solution_rows)) {
			//If it isn't redundant, then add it back to the reduced row set:
			solution_rows.add(row_ind);
		}
		//Pop this row from the back of the unprocessed set:
		unprocessed_rows.remove(row_ind);
	}
	return;
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
	trivial_solution.agreements = 0;
	trivial_solution.cost = numeric_limits<float>::infinity();
	for (set_cover_row row : rows) {
		if (target.isSubset(row.explained) && row.cost < trivial_solution.cost) {
			trivial_solution.rows = list<set_cover_row>({row});
			trivial_solution.agreements = row.agreements.cardinality();
			trivial_solution.cost = row.cost;
		}
	}
	return trivial_solution;
}

/**
 * Returns the set cover solution found by the basic greedy heuristic.
 */
set_cover_solution set_cover_solver::get_greedy_solution() const {
	Roaring greedy_solution_rows = Roaring();
	Roaring uncovered = Roaring(target);
	//Until the target is completely covered, choose the row with the lowest cost-to-coverage proportion:
	while (!uncovered.isEmpty()) {
		float best_density = numeric_limits<float>::infinity();
		unsigned int best_row_ind = 0;
		unsigned int row_ind = 0;
		for (set_cover_row row : rows) {
			float cost = row.cost;
			float coverage = float((uncovered & row.explained).cardinality());
			//Skip if there is no coverage:
			if (coverage == 0) {
				row_ind++;
				continue;
			}
			float density = cost / coverage;
			if (density < best_density && !greedy_solution_rows.contains(row_ind)) {
				best_density = density;
				best_row_ind = row_ind;
			}
			row_ind++;
		}
		//Add the best-found row to the initial solution, and remove its overlap with the target set from the target set:
		greedy_solution_rows.add(best_row_ind);
		set_cover_row best_row = rows[best_row_ind];
		uncovered ^= uncovered & best_row.explained;
	}
	//Now remove any redundant columns from this solution:
	remove_redundant_rows_from_solution(greedy_solution_rows);
	set_cover_solution greedy_solution = get_solution_from_rows(greedy_solution_rows);
	return greedy_solution;
}

/**
 * Given a bitmap representing rows remaining to be processed and a stack of branch-and-bound nodes,
 * adds the a candidate solution node for the next row to the stack.
 */
void set_cover_solver::branch(const Roaring & remaining, stack<branch_and_bound_node> & nodes) {
	//If there are no remaining rows, then do nothing:
	if (remaining.isEmpty()) {
		return;
	}
	//Otherwise, find the first remaining row:
	unsigned int row_ind = remaining.minimum();
	//Add a node for it:
	branch_and_bound_node node;
	node.row = row_ind;
	node.state = node_state::ACCEPT;
	nodes.push(node);
	return;
}

/**
 * Given a bitmap of solution rows,
 * returns a lower bound on the cost of any solution that contains those rows.
 */
float set_cover_solver::bound(const Roaring & solution_rows) const {
	float bound = 0;
	for (Roaring::const_iterator it = solution_rows.begin(); it != solution_rows.end(); it++) {
		unsigned int row_ind = *it;
		set_cover_row row = rows[row_ind];
		bound += row.cost;
	}
	return bound;
}

/**
 * Populates a list of set cover solutions via branch and bound.
 * If the set cover solver was constructed without a fixed upper bound, then the map will consist only of solutions with the lowest cost.
 * If the set cover solver was constructed with a fixed upper bound, then this method will enumerate all solutions with costs within that bound.
 */
void set_cover_solver::branch_and_bound(list<set_cover_solution> & solutions) {
	//Initialize a map of solution row set bitmaps, keyed by their serializations:
	unordered_map<string, Roaring> distinct_row_sets = unordered_map<string, Roaring>();
	//Initialize bitmaps representing rows included in the current solution and rows to be processed:
	Roaring accepted = Roaring();
	Roaring remaining = Roaring();
	remaining.addRange(0, rows.size());
	//Initialize a stack of branch-and-bound nodes:
	stack<branch_and_bound_node> nodes = stack<branch_and_bound_node>();
	//If no fixed upper bound is specified, then obtain a good initial upper bound quickly using the trivial solution and the greedy solution:
	float ub = fixed_ub;
	bool is_ub_fixed = fixed_ub < numeric_limits<float>::infinity();
	if (!is_ub_fixed) {
		set_cover_solution trivial_solution = get_trivial_solution();
		set_cover_solution greedy_solution = get_greedy_solution();
		ub = min(trivial_solution.cost, greedy_solution.cost);
	}
	//Initialize the stack of branch and bound nodes with the first node:
	branch(remaining, nodes);
	//Then continue with branch and bound until there is nothing left to be processed:
	while (!nodes.empty()) {
		//Get the current node from the stack:
		branch_and_bound_node & node = nodes.top();
		//Adjust the set partitions to reflect the candidate solution representing by the current node:
		unsigned int row = node.row;
		if (node.state == node_state::ACCEPT) {
			//Add the candidate row to the solution:
			remaining.remove(row);
			accepted.add(row);
			//Update its state:
			node.state = node_state::REJECT;
		}
		else if (node.state == node_state::REJECT) {
			//Exclude the candidate row from the solution:
			accepted.remove(row);
			//Update its state:
			node.state = node_state::DONE;
		}
		else {
			//We're done processing this node, and we can add its row back to the set of available rows:
			remaining.add(row);
			nodes.pop();
			continue;
		}
		//Check if current set of accepted rows represents a feasible solution:
		if (is_feasible(accepted)) {
			//If it does, then calculate the cost of the solution:
			Roaring solution_rows = Roaring(accepted);
			//If we're just looking for the minimum-cost solution, then remove redundant rows:
			if (!is_ub_fixed) {
				remove_redundant_rows_from_solution(solution_rows);
			}
			float cost = bound(solution_rows);
			//Check if this cost is within the current upper bound:
			if (cost <= ub) {
				//If it is, then make any necessary updates to the upper bound and solution set if we're just looking for minimum-cost solutions:
				if (!is_ub_fixed && cost < ub) {
					ub = cost;
					distinct_row_sets = unordered_map<string, Roaring>();
				}
				//Then add the solution row bitmap to the solution set:
				string serialized = solution_rows.toString();
				distinct_row_sets[serialized] = solution_rows;
			}
			//If we're just looking for minimum-cost solutions, then branching past this point is unnecessary:
			if (!is_ub_fixed) {
				continue;
			}
		}
		//Check if there is any feasible solution under the current node:
		if (is_feasible(accepted | remaining)) {
			//Lower-bound the cost of any solution under the current node:
			float lb = bound(accepted);
			//If this lower bound is within the upper bound, then branch on this node:
			if (lb <= ub) {
				branch(remaining, nodes);
			}
		}
	}
	//For each distinct set of solution rows, add a set cover solution data structure to the solutions list:
	for (pair<string, Roaring> kv : distinct_row_sets) {
		Roaring solution_rows = kv.second;
		set_cover_solution solution = get_solution_from_rows(solution_rows);
		solutions.push_back(solution);
	}
	return;
}

/**
 * Populates the given solution list with solutions to the set cover problem.
 * If the set cover solver was constructed with a fixed upper bound, then this method will enumerate all solutions with costs within that bound.
 */
void set_cover_solver::solve(list<set_cover_solution> & solutions) {
	solutions = list<set_cover_solution>();
	//Create a map of row IDs to their indices:
	unordered_map<string, unsigned int> row_ids_to_inds = unordered_map<string, unsigned int>();
	unsigned int row_ind = 0;
	for (set_cover_row row : rows) {
		string row_id = row.id;
		row_ids_to_inds[row_id] = row_ind;
		row_ind++;
	}
	//If any column cannot be covered by the rows provided, then we're done:
	if (!get_uncovered_columns().isEmpty()) {
		return;
	}
	//If any rows uniquely cover one or more columns, then those rows must be set aside to be included in the solution:
	Roaring unique_rows = get_unique_rows();
	//Reduce the current problem to an easier subproblem by removing all columns covered by the unique coverage rows from the target set:
	Roaring subproblem_target = Roaring(target);
	float subproblem_ub = fixed_ub;
	for (Roaring::const_iterator it = unique_rows.begin(); it != unique_rows.end(); it++) {
		unsigned int row_ind = *it;
		set_cover_row row = rows[row_ind];
		subproblem_target ^= subproblem_target & row.explained;
		subproblem_ub -= row.cost;
	}
	//If the total cost of the unique coverage rows exceeds the upper bound, then there is no solution:
	if (subproblem_ub < 0) {
		return;
	}
	//If no columns need to be covered anymore, then the unique coverage rows constitute a feasible solution:
	if (subproblem_target.isEmpty()) {
		set_cover_solution solution = get_solution_from_rows(unique_rows);
		solutions.push_back(solution);
		//If we're just looking for a minimum-cost solution, then this is is the unique lowest-cost solution, and we're done:
		if (fixed_ub == numeric_limits<float>::infinity()) {
			return;
		}
	}
	//Otherwise, solve the subproblem using branch-and-bound:
	vector<set_cover_row> subproblem_rows = vector<set_cover_row>();
	for (set_cover_row row : rows) {
		//If the row has a cost that exceeds the upper bound of the subproblem, then exclude it:
		if (row.cost > subproblem_ub) {
			continue;
		}
		//If we're just looking for a minimum-cost solution,
		//then exclude any rows that have no overlap with the remaining target set:
		if (fixed_ub == numeric_limits<float>::infinity() && (row.explained & target).cardinality() == 0) {
			continue;
		}
		subproblem_rows.push_back(row);
	}
	list<set_cover_solution> subproblem_solutions = list<set_cover_solution>();
	set_cover_solver subproblem_solver = fixed_ub != numeric_limits<float>::infinity() ? set_cover_solver(subproblem_rows, subproblem_target, subproblem_ub) : set_cover_solver(subproblem_rows, subproblem_target);
	subproblem_solver.branch_and_bound(subproblem_solutions);
	//Then add the unique coverage rows found earlier to the subproblem solutions:
	set_cover_solution unique_rows_solution = get_solution_from_rows(unique_rows);
	for (set_cover_solution subproblem_solution : subproblem_solutions) {
		set_cover_solution solution;
		solution.rows = list<set_cover_row>();
		Roaring row_set = Roaring();
		for (set_cover_row row : subproblem_solution.rows) {
			row_set.add(row_ids_to_inds.at(row.id));
		}
		for (set_cover_row row : unique_rows_solution.rows) {
			row_set.add(row_ids_to_inds.at(row.id));
		}
		for (Roaring::const_iterator it = row_set.begin(); it != row_set.end(); it++) {
			unsigned int row_ind = *it;
			set_cover_row row = rows[row_ind];
			solution.rows.push_back(row);
		}
		solution.cost = subproblem_solution.cost + unique_rows_solution.cost;
		Roaring agreements = Roaring();
		for (set_cover_row row : solution.rows) {
			agreements |= row.agreements;
		}
		solution.agreements = agreements.cardinality();
		solutions.push_back(solution);
	}
	//Then sort the solutions:
	solutions.sort([row_ids_to_inds](const set_cover_solution & s1, const set_cover_solution & s2) {
		//Sort first by cost:
		if (s1.cost < s2.cost) {
			return true;
		}
		else if (s1.cost > s2.cost) {
			return false;
		}
		//Then sort by cardinality:
		if (s1.rows.size() < s2.rows.size()) {
			return true;
		}
		else if (s1.rows.size() > s2.rows.size()) {
			return false;
		}
		//Then sort by number of agreements:
		if (s1.agreements > s2.agreements) {
			return true;
		}
		else if (s1.agreements < s2.agreements) {
			return false;
		}
		//Then sort lexicographically by the indices of the rows in the solutions:
		Roaring rs1 = Roaring();
		for (set_cover_row row : s1.rows) {
			rs1.add(row_ids_to_inds.at(row.id));
		}
		Roaring rs2 = Roaring();
		for (set_cover_row row : s2.rows) {
			rs2.add(row_ids_to_inds.at(row.id));
		}
		while (!rs1.isEmpty()) {
			unsigned int rs1_min = rs1.minimum();
			unsigned int rs2_min = rs2.minimum();
			if (rs1_min < rs2_min) {
				return true;
			}
			else if (rs1_min > rs2_min) {
				return false;
			}
			rs1.remove(rs1_min);
			rs2.remove(rs2_min);
		}
		return false;
	});
	return;
}
