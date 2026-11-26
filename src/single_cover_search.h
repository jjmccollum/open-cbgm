#ifndef SINGLE_COVER_SEARCH_H
#define SINGLE_COVER_SEARCH_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <unordered_map>
#include <vector>

#include "set_cover_solver.h"

namespace {

class single_cover_search {
    using bitmap = roaring::Roaring;
    using cost = std::int64_t;

    std::vector<cost> costs;
    std::vector<std::vector<unsigned int>> support;
    std::vector<bitmap> coverage;
    bitmap required;
    bitmap best;
    cost limit = std::numeric_limits<cost>::max();
    unsigned int remaining_nodes;

    // Integer arithmetic is equivalent to the library's float arithmetic
    // when every scaled candidate cost and their full sum are representable.
    bool scale_costs(const std::vector<set_cover_row> & rows) {
        for (int shift = 0; shift <= std::numeric_limits<float>::digits; ++shift) {
            double total = 0;
            bool integral = true;
            for (const auto & row : rows) {
                const double value = std::ldexp(static_cast<double>(row.cost), shift);
                if (!std::isfinite(value) || value <= 0) return false;
                total += value;
                if (total > std::ldexp(1.0, std::numeric_limits<float>::digits)) return false;
                integral &= std::floor(value) == value;
            }
            if (integral) {
                for (const auto & row : rows) costs.push_back(std::ldexp(row.cost, shift));
                return true;
            }
        }
        return false;
    }

    // A passage with fewer possible ancestors makes any passage with a
    // superset of those ancestors redundant in the coverage constraints.
    static std::vector<bitmap> minimal_constraints(const std::set<std::vector<unsigned int>> & unique) {
        std::vector<bitmap> ordered;
        for (const auto & candidates : unique)
            ordered.emplace_back(candidates.size(), candidates.data());
        std::stable_sort(ordered.begin(), ordered.end(), [](const bitmap & a, const bitmap & b) {
            return a.cardinality() < b.cardinality();
        });
        std::vector<bitmap> minimal;
        for (auto & candidates : ordered) {
            if (std::none_of(minimal.begin(), minimal.end(), [&](const bitmap & other) {
                return other.isSubset(candidates);
            })) minimal.push_back(std::move(candidates));
        }
        return minimal;
    }

    void reduce(const std::vector<set_cover_row> & rows, const bitmap & target) {
        std::unordered_map<unsigned int, std::vector<unsigned int>> by_passage;
        for (unsigned int i = 0; i < rows.size(); ++i)
            for (const auto p : (rows[i].explained & target)) by_passage[p].push_back(i);

        std::set<std::vector<unsigned int>> unique;
        for (auto & passage : by_passage) unique.insert(std::move(passage.second));
        const auto constraints = minimal_constraints(unique);
        coverage.resize(rows.size());
        for (unsigned int i = 0; i < constraints.size(); ++i)
            for (const auto candidate : constraints[i]) coverage[candidate].add(i);

        // Leave equal-cost dominated candidates in the model so that the
        // search can detect ties and defer to the legacy tie-breaking rule.
        bitmap dominated;
        for (unsigned int i = 0; i < rows.size(); ++i) {
            if (coverage[i].isEmpty()) continue;
            for (unsigned int j = 0; j < rows.size(); ++j) {
                if (costs[j] < costs[i] && coverage[i].isSubset(coverage[j])) {
                    dominated.add(i);
                    break;
                }
            }
        }
        unique.clear();
        for (const auto & constraint : constraints) {
            const auto candidates = constraint - dominated;
            std::vector<unsigned int> indices;
            for (const auto i : candidates) indices.push_back(i);
            unique.insert(std::move(indices));
        }

        coverage.assign(rows.size(), bitmap());
        const auto reduced = minimal_constraints(unique);
        support.resize(reduced.size());
        required.addRange(0, reduced.size());
        for (unsigned int p = 0; p < reduced.size(); ++p) {
            for (const auto i : reduced[p]) {
                coverage[i].add(p);
                support[p].push_back(i);
            }
        }
    }

    bool search(const bitmap & missing, cost spent, bitmap & selected,
                unsigned int excluded, bool stop_at_first) {
        if (spent >= limit || !remaining_nodes) return false;
        --remaining_nodes;
        if (missing.isEmpty()) {
            limit = spent;
            best = selected;
            return true;
        }

        // Greedily raise dual prices for uncovered constraints. Remaining
        // candidate costs stay nonnegative, so their sum is a lower bound.
        auto residual = costs;
        cost lower = spent;
        const auto no_passage = std::numeric_limits<unsigned int>::max();
        auto branch_passage = no_passage;
        for (const auto p : missing) {
            cost increase = std::numeric_limits<cost>::max();
            for (const auto i : support[p])
                if (i != excluded) increase = std::min(increase, residual[i]);
            if (increase == std::numeric_limits<cost>::max()) return false;
            if (branch_passage == no_passage) branch_passage = p;
            lower += increase;
            if (lower >= limit) return false;
            if (increase > 0)
                for (const auto i : support[p]) residual[i] -= increase;
        }

        auto choices = support[branch_passage];
        std::stable_sort(choices.begin(), choices.end(), [&](unsigned int a, unsigned int b) {
            return costs[a] * coverage[b].and_cardinality(missing) <
                   costs[b] * coverage[a].and_cardinality(missing);
        });
        bool found = false;
        for (const auto i : choices) {
            if (i == excluded) continue;
            selected.add(i);
            const bool improved = search(missing - coverage[i], spent + costs[i], selected,
                                         excluded, stop_at_first);
            selected.remove(i);
            found |= improved;
            if (stop_at_first && found) return true;
        }
        return found;
    }

public:
    explicit single_cover_search(unsigned int node_budget) : remaining_nodes(node_budget) {}

    bool find(const std::vector<set_cover_row> & rows, const bitmap & target,
              const bitmap & greedy, set_cover_tie_policy tie_policy, bitmap & result) {
        if (!scale_costs(rows)) return false;
        reduce(rows, target);
        cost greedy_cost = 0;
        for (const auto i : greedy) greedy_cost += costs[i];
        limit = greedy_cost;
        bitmap selected;
        const auto no_exclusion = std::numeric_limits<unsigned int>::max();
        const bool improved = search(required, 0, selected, no_exclusion, false);
        if (!remaining_nodes) return false;
        if (!improved) {
            result = greedy;
            return true;
        }
        if (tie_policy == set_cover_tie_policy::any_minimum) {
            result = best;
            return true;
        }

        const auto optimum = best;
        const auto optimal_cost = limit;
        // A different minimum-cost set must exclude one selected row,
        // because all costs are strictly positive.
        for (const auto i : optimum) {
            limit = optimal_cost + 1;
            if (search(required, 0, selected, i, true) || !remaining_nodes) return false;
        }
        result = optimum;
        return true;
    }
};

} // namespace

#endif
