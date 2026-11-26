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

namespace {
using clock_type = std::chrono::steady_clock;

double seconds(clock_type::time_point start, clock_type::time_point end) {
    return std::chrono::duration<double>(end - start).count();
}

template <typename Work>
void run_workers(std::size_t size, int workers, Work work) {
    const auto count = std::min(static_cast<std::size_t>(workers), size);
    if (count <= 1) {
        for (std::size_t i = 0; i < size; ++i) work(i);
        return;
    }

    std::atomic<std::size_t> next{0};
    auto run = [&] {
        for (auto i = next.fetch_add(1); i < size; i = next.fetch_add(1)) work(i);
    };
    std::vector<std::thread> threads;
    threads.reserve(count - 1);
    try {
        for (std::size_t i = 1; i < count; ++i) threads.emplace_back(run);
    } catch (const std::system_error &) {
        for (auto & thread : threads) thread.join();
        throw;
    }
    run();
    for (auto & thread : threads) thread.join();
}
}

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
    const auto start = clock_type::now();
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
    const auto witness_ids = app.get_list_wit();
    const std::vector<std::string> ids(witness_ids.begin(), witness_ids.end());
    const auto parsed_at = clock_type::now();

    std::list<witness> witnesses;
    std::vector<std::unique_ptr<textual_flow>> graphs;
    auto compared_at = parsed_at;
    try {
        // Assemble witnesses in collation order without copying their comparison data.
        std::vector<std::list<witness>> pending(ids.size());
        run_workers(ids.size(), workers, [&](std::size_t i) {
            pending[i].emplace_back(ids[i], app);
        });
        for (auto & entry : pending) witnesses.splice(witnesses.end(), entry);
        compared_at = clock_type::now();

        graphs.resize(units.size());
        run_workers(units.size(), workers, [&](std::size_t i) {
            graphs[i].reset(new textual_flow(units[i], witnesses));
        });
    } catch (const std::system_error & error) {
        std::cerr << "Unable to run benchmark workers: " << error.what() << '\n';
        return 1;
    }
    const auto constructed_at = clock_type::now();
    std::cerr << "Parsed and initialized apparatus in " << seconds(start, parsed_at) << " seconds\n"
              << witnesses.size() << " witnesses compared in " << seconds(parsed_at, compared_at) << " seconds\n"
              << units.size() << " graphs in " << seconds(compared_at, constructed_at) << " seconds\n"
              << "Total before serialization: " << seconds(start, constructed_at) << " seconds\n";

    // Write graphs in collation order so different worker counts can be compared.
    const auto write_start = clock_type::now();
    std::cout << std::setprecision(9);
    for (const auto & flow : graphs) {
        std::cout << flow->get_label() << '\n';
        for (const auto & vertex : flow->get_vertices())
            std::cout << vertex.id << '\t' << vertex.rdg << '\n';
        for (const auto & edge : flow->get_edges())
            std::cout << edge.ancestor << '\t' << edge.descendant << '\t'
                      << edge.type << '\t' << edge.connectivity << '\t' << edge.strength << '\n';
    }
    std::cout.flush();
    if (!std::cout) {
        std::cerr << "Unable to write benchmark graph output.\n";
        return 1;
    }
    std::cerr << "Serialized graphs in " << seconds(write_start, clock_type::now()) << " seconds\n";
}
