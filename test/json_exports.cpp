#include <iostream>
#include <sstream>
#include <string>

#include "enumerate_relationships_table.h"
#include "local_stemma.h"

static bool check_json(const char * exporter, const std::string & actual, const std::string & expected) {
	if (actual == expected) {
		return true;
	}
	std::cerr << exporter << "\nExpected: " << expected << "\nActual:   " << actual << '\n';
	return false;
}

int main() {
	local_stemma graph("unit1", "He said \"go\"\n\\\t\b\f\r\x01", {{"a\""}, {"b\\"}}, {{"a\"", "b\\", 1}});
	std::ostringstream graph_json;
	graph.to_json(graph_json);
	const bool graph_ok = check_json("Local stemma", graph_json.str(),
		R"json({"label":"He said \"go\"\n\\\t\b\f\r\u0001","vertices":[{"id":"a\""},{"id":"b\\"}],"edges":[{"prior":"a\"","posterior":"b\\","weight":1}]})json");

	genealogical_comparison comparison{};
	comparison.primary_wit = u8"\u03B1\"";
	comparison.secondary_wit = "B\\";
	comparison.extant = roaring::Roaring::bitmapOf(2, 0, 1);
	enumerate_relationships_table table(comparison, {"unit\t1", "unit\t1"});

	std::ostringstream table_json;
	table.to_json(table_json, {"extant", "agree", "unknown"});
	const std::string witnesses = "{\"primary_wit\":\"" + std::string(u8"\u03B1") + R"json(\"","secondary_wit":"B\\")json";
	const bool table_ok = check_json("Filtered relationships", table_json.str(),
		witnesses + R"json(,"extant":["unit\t1","unit\t1"],"agree":[]})json");

	std::ostringstream metadata_json;
	table.to_json(metadata_json, {});
	const bool metadata_ok = check_json("Unfiltered relationship metadata", metadata_json.str(), witnesses + "}");

	return graph_ok && table_ok && metadata_ok ? 0 : 1;
}
