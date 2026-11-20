#ifndef WITNESS_COMPARISON_INDEX_H
#define WITNESS_COMPARISON_INDEX_H

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "variation_unit.h"

struct indexed_attestation {
	std::size_t passage;
	std::size_t reading;
};

struct reading_relationship {
	bool agrees = false;
	bool prior = false;
	bool posterior = false;
	bool common_ancestor = false;
	bool classic_explained = false;
	float weight = 0;
};

class witness_comparison_index {
	struct indexed_passage {
		std::vector<std::string> readings;
	};

	std::unordered_map<std::string, std::vector<indexed_attestation>> attestations;
	std::vector<indexed_passage> passages;

public:
	explicit witness_comparison_index(const std::vector<variation_unit> & units);
	const std::vector<indexed_attestation> & attestations_for(const std::string & witness_id) const;
	reading_relationship relationship(const indexed_attestation & primary, const indexed_attestation & secondary,
		const local_stemma & graph) const;
};

class witness_comparison_cache {
	std::once_flag initialized;
	std::unique_ptr<const witness_comparison_index> index;

public:
	const witness_comparison_index & get(const std::vector<variation_unit> & units);
};

#endif
