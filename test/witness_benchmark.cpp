#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

#include "witness.h"

int main(int argc, char ** argv) {
	const bool classic = argc == 3 && std::string(argv[2]) == "--classic";
	if (argc != 2 && !classic) {
		std::cerr << "Usage: witness_benchmark COLLATION.xml [--classic]\n";
		return 1;
	}
	pugi::xml_document doc;
	const auto parsed = doc.load_file(argv[1]);
	if (!parsed) {
		std::cerr << argv[1] << ": " << parsed.description() << '\n';
		return 1;
	}
	const apparatus app(doc.child("TEI"), false, {}, {}, {});
	const auto ids = app.get_list_wit();
	if (ids.empty()) {
		std::cerr << argv[1] << ": no witnesses found\n";
		return 1;
	}

	using clock = std::chrono::steady_clock;
	clock::duration elapsed{};
	std::cout << std::setprecision(std::numeric_limits<float>::max_digits10);
	for (const auto & primary : ids) {
		const auto start = clock::now();
		const witness wit(primary, app, classic);
		elapsed += clock::now() - start;

		// Print results in collation order so runs can be compared with diff.
		for (const auto & secondary : ids) {
			const auto comp = wit.get_genealogical_comparison_for_witness(secondary);
			std::cout << comp.primary_wit << '\t' << comp.secondary_wit << '\t'
				<< comp.extant.toString() << '\t' << comp.agreements.toString() << '\t'
				<< comp.prior.toString() << '\t' << comp.posterior.toString() << '\t'
				<< comp.norel.toString() << '\t' << comp.unclear.toString() << '\t'
				<< comp.explained.toString() << '\t' << comp.cost << '\n';
		}
		std::cout << "Potential ancestors for " << primary;
		for (const auto & ancestor : wit.get_potential_ancestor_ids()) {
			std::cout << '\t' << ancestor;
		}
		std::cout << '\n';
	}
	std::cerr << "Constructed " << ids.size() << " witnesses in "
		<< std::chrono::duration<double>(elapsed).count() << " seconds\n";
	return 0;
}
