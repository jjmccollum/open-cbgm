#include <iostream>
#include "witness.h"

int main() {
    pugi::xml_document xml;
    if (!xml.load_string(R"(
<TEI>
  <teiHeader><fileDesc><sourceDesc><listWit>
    <witness n="A"/><witness n="B"/><witness n="C"/><witness n="D"/>
  </listWit></sourceDesc></fileDesc></teiHeader>
  <text><body><app n="p1">
    <note><graph>
      <node n="a-1"/><node n="a-2"/><node n="a-3"/><node n="b-1"/>
    </graph></note>
    <rdg n="a-1" wit="A">first</rdg>
    <rdg n="a-2" wit="B" type="split">first</rdg>
    <rdg n="a-3" wit="C" type="split">first</rdg>
    <rdg n="b-1" wit="D">second</rdg>
  </app></body></text>
</TEI>)")) return 1;

    for (bool merge_splits : {false, true}) {
        const apparatus app(xml.child("TEI"), merge_splits, {}, {}, {});
        const auto unit = app.get_variation_units().front();
        const auto support = unit.get_reading_support();
        const auto graph = unit.get_local_stemma();
        if (support.at("A") != "a-1" || support.at("B") != "a-2" ||
            support.at("C") != "a-3" || support.at("D") != "b-1" ||
            graph.readings_agree("a-1", "a-2") != merge_splits ||
            graph.readings_agree("a-2", "a-3") != merge_splits ||
            graph.readings_agree("a-1", "b-1")) {
            std::cerr << "Unexpected local stemma or witness assignments\n";
            return 1;
        }

        const witness w("B", app);
        const auto same_label = w.get_genealogical_comparison_for_witness("A");
        const auto different_label = w.get_genealogical_comparison_for_witness("D");
        if (!same_label.extant.contains(0) ||
            same_label.agreements.contains(0) != merge_splits ||
            same_label.explained.contains(0) != merge_splits ||
            different_label.agreements.contains(0) || different_label.explained.contains(0)) {
            std::cerr << "Unexpected agreement or coverage for a split attestation\n";
            return 1;
        }
    }
}
