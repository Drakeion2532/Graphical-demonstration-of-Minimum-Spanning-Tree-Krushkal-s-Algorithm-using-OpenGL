#include <cassert>
#include <cmath>
#include <iostream>
#include <random>

#include "../src/kruskal.hpp"

static int checks = 0;
#define CHECK(c) do { assert(c); checks++; } while (0)

int main() {
    // 1. textbook example has a known answer
    Graph clrs = load_graph(DATA_DIR "/clrs_example.txt");
    KruskalResult r = kruskal(clrs);
    CHECK(r.spanning);
    CHECK(r.total == 37);
    CHECK(r.tree.size() == 8);

    // 2. union-find basics
    DSU d(5);
    CHECK(d.components() == 5);
    CHECK(d.unite(0, 1) && d.unite(1, 2));
    CHECK(!d.unite(0, 2));             // would form a cycle
    CHECK(d.components() == 3);

    // 3. Kruskal == Prim on 500 random graphs (random integer weights, many ties)
    std::mt19937 rng(123);
    for (int t = 0; t < 500; t++) {
        Graph g;
        int n = 2 + static_cast<int>(rng() % 30);
        g.nodes.resize(n);
        int m = static_cast<int>(rng() % (n * 3 + 1));
        for (int i = 0; i < m; i++) g.edges.push_back({static_cast<int>(rng() % n), static_cast<int>(rng() % n), static_cast<double>(rng() % 20)});
        bool connected;
        double p = prim_total(g, &connected);
        KruskalResult k = kruskal(g, false);
        CHECK(std::fabs(p - k.total) < 1e-9);
        CHECK(k.spanning == connected);
        CHECK(k.steps.empty() ? k.tree.empty() : static_cast<int>(k.tree.size()) == n - k.steps.back().components);
    }

    // 4. disconnected graph -> forest, flagged
    Graph two;
    two.nodes.resize(4);
    two.edges = {{0, 1, 3}, {2, 3, 5}};
    KruskalResult f = kruskal(two);
    CHECK(!f.spanning && f.total == 8 && f.steps.back().components == 2);

    // 5. self-loops are always rejected
    Graph loop;
    loop.nodes.resize(2);
    loop.edges = {{0, 0, 1}, {0, 1, 5}};
    KruskalResult l = kruskal(loop);
    CHECK(l.total == 5 && l.steps.front().kind == StepKind::RejectCycle);

    // 6. real-world file loads and connects every city
    Graph india = load_graph(DATA_DIR "/india_fibre_network.txt");
    KruskalResult ir = kruskal(india);
    CHECK(india.nodes.size() == 14 && ir.spanning && ir.tree.size() == 13);

    // 7. random generator always gives a connected graph
    for (unsigned s = 1; s <= 50; s++) CHECK(kruskal(random_graph(15, 0.5, s)).spanning);

    std::cout << "all " << checks << " checks passed\n";
    return 0;
}
