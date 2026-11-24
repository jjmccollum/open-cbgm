#include <chrono>
#include <iomanip>
#include <iostream>
#include <list>
#include "textual_flow.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: textual_flow_benchmark <collation.xml>\n";
        return 2;
    }
    pugi::xml_document xml;
    const auto parsed = xml.load_file(argv[1]);
    if (!parsed) {
        std::cerr << argv[1] << ": " << parsed.description() << '\n';
        return 1;
    }
    const auto tei = xml.child("TEI");
    if (!tei) {
        std::cerr << "Expected a TEI root element.\n";
        return 1;
    }
    const apparatus app(tei, false, {}, {}, {});
    const auto units = app.get_variation_units();
    std::list<witness> witnesses;
    for (const auto & id : app.get_list_wit()) {
        witnesses.emplace_back(id, app);
    }
    using clock = std::chrono::steady_clock;
    clock::duration elapsed{};
    // Serialize the ordered graph outside the timed section for output comparison.
    std::cout << std::setprecision(9);
    for (const auto & unit : units) {
        const auto start = clock::now();
        textual_flow flow(unit, witnesses);
        elapsed += clock::now() - start;
        std::cout << flow.get_label() << '\n';
        for (const auto & vertex : flow.get_vertices())
            std::cout << vertex.id << '\t' << vertex.rdg << '\n';
        for (const auto & edge : flow.get_edges())
            std::cout << edge.ancestor << '\t' << edge.descendant << '\t'
                      << edge.type << '\t' << edge.connectivity << '\t' << edge.strength << '\n';
    }
    std::cerr << units.size() << " graphs in "
              << std::chrono::duration<double>(elapsed).count() << " seconds\n";
}
