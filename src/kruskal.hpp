// kruskal.hpp — graph model, union-find and a step-by-step Kruskal's algorithm.
// No OpenGL in here: the algorithm is tested and reused by the CLI and the viewer.
#pragma once

#include <string>
#include <vector>

struct Node {
    std::string name;
    double x = 0, y = 0;      // position on the canvas (0..1 each)
};

struct Edge {
    int u = 0, v = 0;
    double w = 0;
};

struct Graph {
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    std::string title;
};

// Disjoint-set union with path compression + union by rank:
// near-constant time per operation (inverse Ackermann).
class DSU {
public:
    explicit DSU(int n = 0);
    int find(int x);
    bool unite(int a, int b);   // false if already in the same set (edge would form a cycle)
    int components() const { return comps_; }
private:
    std::vector<int> parent_, rank_;
    int comps_ = 0;
};

enum class StepKind { Accept, RejectCycle };

struct Step {
    Edge edge;
    StepKind kind;
    double total;             // MST cost after this step
    int components;           // number of separate trees after this step
    std::vector<int> comp;    // component id of every node after this step (for colouring)
};

struct KruskalResult {
    std::vector<Step> steps;  // every edge considered, in order
    std::vector<Edge> tree;   // accepted edges
    double total = 0;
    bool spanning = false;    // false => graph is disconnected, result is a forest
};

// Edges are processed by (weight, u, v) so ties are deterministic.
KruskalResult kruskal(const Graph& g, bool stop_when_spanning = true);

// Independent O(V^2) Prim's algorithm, used by tests to verify Kruskal's total.
double prim_total(const Graph& g, bool* connected = nullptr);

// Graph helpers
Graph load_graph(const std::string& path);                 // throws std::runtime_error
Graph random_graph(int n, double density, unsigned seed);  // connected, Euclidean weights
void connect_complete(Graph& g, double scale);             // all pairs, weight = distance * scale
std::string fmt_weight(double w);
