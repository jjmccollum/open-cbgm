#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include "witness.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        std::cerr << "Usage: substemma_benchmark <collation.xml> <workers> <seconds-per-witness, 0 for no cap>\n";
        return 2;
    }
    int workers, limit;
    try {
        std::size_t workers_end, limit_end;
        workers = std::stoi(argv[2], &workers_end);
        limit = std::stoi(argv[3], &limit_end);
        if (workers < 1 || limit < 0 || argv[2][workers_end] || argv[3][limit_end]) return 2;
    } catch (const std::logic_error &) {
        return 2;
    }
    pugi::xml_document xml;
    if (!xml.load_file(argv[1]) || !xml.child("TEI")) return 1;
    const apparatus app(xml.child("TEI"), false, {}, {}, {});
    const auto listed = app.get_list_wit();
    const std::vector<std::string> ids(listed.begin(), listed.end());
    std::vector<std::unique_ptr<witness>> witnesses(ids.size());

    using clock = std::chrono::steady_clock;
    const auto prepare_start = clock::now();
    std::atomic<std::size_t> next{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < workers; ++i) {
        threads.emplace_back([&] {
            for (auto n = next.fetch_add(1); n < ids.size(); n = next.fetch_add(1))
                witnesses[n].reset(new witness(ids[n], app));
        });
    }
    for (auto & thread : threads) thread.join();
    std::cerr << "initialisation_seconds " << std::chrono::duration<double>(clock::now() - prepare_start).count()
              << " witnesses " << ids.size() << '\n';

    const auto search_start = clock::now();
    std::unordered_map<pid_t, std::size_t> children;
    unsigned int solved = 0, infeasible = 0, timeouts = 0;
    long peak_rss = 0;
    for (std::size_t i = 0; i < ids.size() || !children.empty();) {
        while (i < ids.size() && children.size() < static_cast<std::size_t>(workers)) {
            const auto pid = fork();
            if (pid < 0) return 1;
            if (pid == 0) {
                if (limit) alarm(limit);
                const auto answer = witnesses[i]->get_substemmata(0, true);
                alarm(0);
                std::ostringstream line;
                line << ids[i] << '\t' << (answer.empty() ? "no-solution" : "solved");
                if (!answer.empty()) {
                    line << '\t' << std::setprecision(9) << answer.front().cost << '\t' << answer.front().agreements;
                    for (const auto & row : answer.front().rows) line << '\t' << row.id;
                }
                line << '\n';
                const auto output = line.str();
                bool success = write(STDOUT_FILENO, output.data(), output.size()) == static_cast<ssize_t>(output.size());
                _exit(success ? (answer.empty() ? 2 : 0) : 3);
            }
            children.emplace(pid, i++);
        }
        int status;
        struct rusage usage;
        const auto pid = wait4(-1, &status, 0, &usage);
        if (pid < 0) return 1;
        peak_rss = std::max(peak_rss, usage.ru_maxrss);
        if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM) {
            ++timeouts;
            std::cout << ids[children.at(pid)] << "\ttimeout" << std::endl;
        } else if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            ++solved;
        } else if (WIFEXITED(status) && WEXITSTATUS(status) == 2) {
            ++infeasible;
        } else {
            return 1;
        }
        children.erase(pid);
    }
    struct rusage parent;
    if (getrusage(RUSAGE_SELF, &parent) != 0) return 1;
    std::cerr << "search_seconds " << std::chrono::duration<double>(clock::now() - search_start).count()
              << " solved " << solved << " no_solution " << infeasible << " timeout " << timeouts
              << " peak_rss_kib " << std::max(peak_rss, parent.ru_maxrss) << '\n';
}
