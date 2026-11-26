#include <iostream>
#include <list>
#include <string>
#include <vector>
#include "set_cover_solver.h"

int main() {
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
