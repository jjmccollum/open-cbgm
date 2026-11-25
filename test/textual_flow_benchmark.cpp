#include <algorithm>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <vector>
#include "textual_flow.h"

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: textual_flow_benchmark <collation.xml> [workers]\n";
        return 2;
    }
    int workers = 1;
    if (argc == 3) {
        std::size_t consumed = 0;
        const std::string count = argv[2];
        try {
            workers = std::stoi(count, &consumed);
        } catch (const std::logic_error &) {
            workers = 0;
        }
        if (workers < 1 || consumed != count.size()) {
            std::cerr << "Expected a positive integer worker count.\n";
            return 2;
        }
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
    const auto & shared_witnesses = witnesses;
    std::vector<std::unique_ptr<textual_flow>> graphs(units.size());
    std::atomic<std::size_t> next{0};
    auto construct = [&] {
        for (auto i = next.fetch_add(1); i < units.size(); i = next.fetch_add(1)) {
            graphs[i].reset(new textual_flow(units[i], shared_witnesses));
        }
    };
    using clock = std::chrono::steady_clock;
    const auto start = clock::now();
    if (workers == 1) {
        construct();
    } else {
        const auto count = std::min(static_cast<std::size_t>(workers), units.size());
        std::vector<std::thread> threads;
        threads.reserve(count);
        try {
            for (std::size_t i = 0; i < count; ++i) {
                threads.emplace_back(construct);
            }
        } catch (const std::system_error & error) {
            for (auto & thread : threads) thread.join();
            std::cerr << "Unable to start benchmark worker: " << error.what() << '\n';
            return 1;
        }
        for (auto & thread : threads) thread.join();
    }
    const auto elapsed = clock::now() - start;
    std::cerr << units.size() << " graphs in "
              << std::chrono::duration<double>(elapsed).count() << " seconds\n";

    // Write graphs in collation order so different worker counts can be compared.
    std::cout << std::setprecision(9);
    for (const auto & flow : graphs) {
        std::cout << flow->get_label() << '\n';
        for (const auto & vertex : flow->get_vertices())
            std::cout << vertex.id << '\t' << vertex.rdg << '\n';
        for (const auto & edge : flow->get_edges())
            std::cout << edge.ancestor << '\t' << edge.descendant << '\t'
                      << edge.type << '\t' << edge.connectivity << '\t' << edge.strength << '\n';
    }
}
