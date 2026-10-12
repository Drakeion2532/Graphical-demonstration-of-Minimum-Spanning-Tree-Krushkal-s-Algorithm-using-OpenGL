// kruskal_cli — runs Kruskal's algorithm without graphics and prints every step.
//   ./kruskal_cli data/clrs_example.txt
//   ./kruskal_cli --random 12 --seed 7
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "kruskal.hpp"

int main(int argc, char** argv) {
    Graph g;
    try {
        if (argc >= 3 && std::strcmp(argv[1], "--random") == 0) {
            unsigned seed = argc >= 5 ? static_cast<unsigned>(std::atoi(argv[4])) : 1;
            g = random_graph(std::atoi(argv[2]), 0.5, seed);
        } else if (argc >= 2) {
            g = load_graph(argv[1]);
        } else {
            std::cerr << "usage: kruskal_cli <graph.txt> | --random N [--seed S]\n";
            return 2;
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    std::cout << (g.title.empty() ? "Graph" : g.title) << ": " << g.nodes.size() << " nodes, " << g.edges.size() << " edges\n\n";
    KruskalResult r = kruskal(g);
    std::cout << std::left << std::setw(6) << "step" << std::setw(26) << "edge (sorted by weight)" << std::setw(10) << "weight"
              << std::setw(22) << "decision" << std::setw(10) << "total" << "trees\n";
    int i = 1;
    for (const Step& s : r.steps) {
        std::string e = g.nodes[s.edge.u].name + " - " + g.nodes[s.edge.v].name;
        std::cout << std::setw(6) << i++ << std::setw(26) << e << std::setw(10) << fmt_weight(s.edge.w)
                  << std::setw(22) << (s.kind == StepKind::Accept ? "ACCEPT" : "reject (cycle)") << std::setw(10)
                  << fmt_weight(s.total) << s.components << "\n";
    }
    std::cout << "\n" << (r.spanning ? "Minimum spanning tree" : "Graph is disconnected: minimum spanning forest")
              << " weight = " << fmt_weight(r.total) << " using " << r.tree.size() << " of " << g.edges.size() << " edges\n";
    return 0;
}
