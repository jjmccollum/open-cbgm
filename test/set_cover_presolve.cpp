#include <iostream>
#include <list>
#include <string>
#include <vector>
#include "set_cover_solver.h"

struct set_cover_solver_test_access {
    static void solve_with_node_budget(set_cover_solver & solver,
            std::list<set_cover_solution> & solutions, unsigned int node_budget) {
        solver.branch_and_bound_single_solution(solutions, set_cover_tie_policy::any_minimum, node_budget);
    }
};

static bool test_exhausted_node_budget() {
    const roaring::Roaring target{0, 1, 2, 3, 4, 5, 6};
    std::vector<set_cover_row> rows = {
        {"A", {}, {4, 5}, 5},
        {"B", {}, {0, 1, 2, 6}, 9},
        {"C", {}, {1, 2}, 5},
        {"D", {}, {3, 6}, 4},
        {"E", {}, {0, 4, 6}, 8},
        {"F", {}, {1, 3, 5, 6}, 6},
        {"G", {}, {0, 4, 5, 6}, 8},
        {"H", {}, {4}, 5},
        {"I", {}, {}, 3},
        {"expensive", {}, target, 30},
    };
    // Pad to the size threshold for the compressed search.
    for (unsigned int i = 0; i < 6; ++i)
        rows.push_back({"dominated-" + std::to_string(i), {}, target, 40});

    set_cover_solver solver(rows, target);
    std::list<set_cover_solution> solutions;
    // Five nodes improve the greedy cost from 19 to 18. The true optimum is 17.
    set_cover_solver_test_access::solve_with_node_budget(solver, solutions, 5);
    if (solutions.size() != 1 || solutions.front().cost != 17) return false;
    std::list<std::string> chosen;
    for (const auto & row : solutions.front().rows) chosen.push_back(row.id);
    return chosen == std::list<std::string>{"C", "D", "G"};
}

int main() {
    if (!test_exhausted_node_budget()) {
        std::cerr << "Expected the exact fallback to improve the uncertified cover\n";
        return 1;
    }
    for (const auto copies : {1U, 4U}) {
        roaring::Roaring target;
        target.addRange(0, 5 * copies);
        auto repeated = [copies](std::initializer_list<unsigned int> columns) {
            roaring::Roaring coverage;
            for (const auto column : columns)
                for (unsigned int copy = 0; copy < copies; ++copy) coverage.add(copy * 5 + column);
            return coverage;
        };
        for (const auto scale : {1.0f, 0.1f}) {
            for (const auto tied : {false, true}) {
                const auto optimum = 1.5f * scale;
                std::vector<set_cover_row> rows = {
                    {"A", {}, repeated({1, 2}), 0.5f * scale},
                    {"B", {}, repeated({0}), scale},
                    {"C", {}, repeated({1, 3, 4}), scale},
                    {"D", target, target, optimum},
                    {"E", target, tied ? target : repeated({0, 1, 3, 4}), optimum},
                };
                for (unsigned int i = 0; i < 11; ++i)
                    rows.push_back({"dominated-" + std::to_string(i), {}, repeated({0, 1}), 4 * scale});

                std::list<set_cover_solution> single;
                set_cover_solver(rows, target, 0.1f).solve(single, true);
                if (single.size() != 1 || single.front().cost != optimum ||
                    single.front().agreements != static_cast<int>(target.cardinality()) || single.front().rows.size() != 1 ||
                    single.front().rows.front().id != "D") {
                    std::cerr << "Unexpected single minimum or tied witness\n";
                    return 1;
                }

                std::list<set_cover_solution> any;
                set_cover_solver(rows, target, 0.1f).solve(any, true, set_cover_tie_policy::any_minimum);
                if (any.size() != 1 || any.front().cost != optimum ||
                    any.front().rows.size() != 1 || !target.isSubset(any.front().rows.front().explained) ||
                    (any.front().rows.front().id != "D" && (!tied || any.front().rows.front().id != "E"))) {
                    std::cerr << "Unexpected minimum with the opt-in tie policy\n";
                    return 1;
                }

                std::list<set_cover_solution> bounded;
                set_cover_solver(rows, target, optimum).solve(bounded);
                std::list<std::string> actual;
                for (const auto & solution : bounded) {
                    if (solution.cost != optimum || solution.rows.size() != 1) return 1;
                    actual.push_back(solution.rows.front().id);
                }
                const std::list<std::string> expected = tied ? std::list<std::string>{"D", "E"} :
                                                             std::list<std::string>{"D"};
                if (actual != expected) {
                    std::cerr << "Unexpected bounded enumeration at an inclusive fractional limit\n";
                    return 1;
                }
            }
        }
    }
}
