#ifndef WITNESS_COMPARISON_INDEX_H
#define WITNESS_COMPARISON_INDEX_H

#include <string>

#include "local_stemma.h"

struct reading_relationship {
	bool agrees = false;
	bool prior = false;
	bool posterior = false;
	bool common_ancestor = false;
	bool classic_explained = false;
	float weight = 0;
};

inline reading_relationship classify_readings(const local_stemma & graph, const std::string & primary, const std::string & secondary) {
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

#endif
