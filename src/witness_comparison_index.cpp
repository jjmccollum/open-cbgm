#include "witness_comparison_index.h"

using namespace std;

static reading_relationship classify_readings(const local_stemma & graph, const string & primary, const string & secondary) {
	reading_relationship relation;
	relation.agrees = graph.readings_agree(primary, secondary);
	if (relation.agrees) {
		return relation;
	}
	//Otherwise, because we allow for cycles in the local stemma, it is necessary to check for a non-trivial path between the readings in both directions:
	relation.prior = graph.path_exists(primary, secondary);
	relation.posterior = graph.path_exists(secondary, primary);
	if (relation.posterior) {
		const auto path = graph.get_path(secondary, primary);
		relation.weight = path.weight;
		relation.classic_explained = path.cardinality <= 1;
	}
	//If the readings have no path connecting them in either direction, then check if they have a common ancestor:
	if (!relation.prior && !relation.posterior) {
		relation.common_ancestor = graph.common_ancestor_exists(primary, secondary);
	}
	return relation;
}

witness_comparison_index::witness_comparison_index(const vector<variation_unit> & units) {
	passages.reserve(units.size());
	for (const auto & unit : units) {
		const size_t position = passages.size();
		passages.emplace_back();
		auto & passage = passages.back();
		unordered_map<string, size_t> reading_indices;
		for (const auto & attestation : unit.reading_support) {
			const auto entry = reading_indices.emplace(attestation.second, passage.readings.size());
			if (entry.second) {
				passage.readings.push_back(attestation.second);
			}
			attestations[attestation.first].push_back({position, entry.first->second});
		}
	}
}

const vector<indexed_attestation> & witness_comparison_index::attestations_for(const string & witness_id) const {
	static const vector<indexed_attestation> empty;
	const auto found = attestations.find(witness_id);
	return found == attestations.end() ? empty : found->second;
}

reading_relationship witness_comparison_index::relationship(const indexed_attestation & primary,
		const indexed_attestation & secondary, const local_stemma & graph) const {
	const auto & passage = passages[primary.passage];
	return classify_readings(graph, passage.readings[primary.reading], passage.readings[secondary.reading]);
}

const witness_comparison_index & witness_comparison_cache::get(const vector<variation_unit> & units) {
	// Several workers can start comparing witnesses from the same apparatus at once.
	call_once(initialized, [&] {
		index.reset(new witness_comparison_index(units));
	});
	return *index;
}
