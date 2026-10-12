#include "kruskal.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

// ------------------------------------------------------------------ DSU
DSU::DSU(int n) : parent_(n), rank_(n, 0), comps_(n) { std::iota(parent_.begin(), parent_.end(), 0); }

int DSU::find(int x) {
    while (parent_[x] != x) {
        parent_[x] = parent_[parent_[x]];   // path halving
        x = parent_[x];
    }
    return x;
}

bool DSU::unite(int a, int b) {
    a = find(a); b = find(b);
    if (a == b) return false;
    if (rank_[a] < rank_[b]) std::swap(a, b);
    parent_[b] = a;
    if (rank_[a] == rank_[b]) rank_[a]++;
    comps_--;
    return true;
}

// -------------------------------------------------------------- Kruskal
KruskalResult kruskal(const Graph& g, bool stop_when_spanning) {
    const int n = static_cast<int>(g.nodes.size());
    std::vector<Edge> es = g.edges;
    std::sort(es.begin(), es.end(), [](const Edge& a, const Edge& b) {
        if (a.w != b.w) return a.w < b.w;
        if (a.u != b.u) return a.u < b.u;
        return a.v < b.v;
    });
    KruskalResult r;
    DSU dsu(n);
    for (const Edge& e : es) {
        if (stop_when_spanning && n > 0 && static_cast<int>(r.tree.size()) == n - 1) break;
        Step s;
        s.edge = e;
        if (e.u != e.v && dsu.unite(e.u, e.v)) {
            s.kind = StepKind::Accept;
            r.tree.push_back(e);
            r.total += e.w;
        } else {
            s.kind = StepKind::RejectCycle;
        }
        s.total = r.total;
        s.components = dsu.components();
        s.comp.resize(n);
        for (int i = 0; i < n; i++) s.comp[i] = dsu.find(i);
        r.steps.push_back(std::move(s));
    }
    r.spanning = n <= 1 || static_cast<int>(r.tree.size()) == n - 1;
    return r;
}

// ----------------------------------------------------------------- Prim
double prim_total(const Graph& g, bool* connected) {
    const int n = static_cast<int>(g.nodes.size());
    const double INF = std::numeric_limits<double>::infinity();
    std::vector<std::vector<double>> w(n, std::vector<double>(n, INF));
    for (const Edge& e : g.edges)
        if (e.u != e.v) { w[e.u][e.v] = std::min(w[e.u][e.v], e.w); w[e.v][e.u] = w[e.u][e.v]; }
    std::vector<double> best(n, INF);
    std::vector<bool> in(n, false);
    double total = 0;
    bool ok = true;
    if (n) best[0] = 0;
    for (int it = 0; it < n; it++) {
        int u = -1;
        for (int i = 0; i < n; i++) if (!in[i] && (u == -1 || best[i] < best[u])) u = i;
        if (best[u] == INF) { ok = false; best[u] = 0; }   // new component: start a fresh tree
        in[u] = true;
        total += best[u];
        for (int v = 0; v < n; v++) if (!in[v] && w[u][v] < best[v]) best[v] = w[u][v];
    }
    if (connected) *connected = ok;
    return total;
}

// -------------------------------------------------------------- helpers
std::string fmt_weight(double w) {
    std::ostringstream os;
    if (std::fabs(w - std::round(w)) < 1e-9) os << static_cast<long long>(std::llround(w));
    else { os.setf(std::ios::fixed); os.precision(1); os << w; }
    return os.str();
}

void connect_complete(Graph& g, double scale) {
    g.edges.clear();
    for (size_t i = 0; i < g.nodes.size(); i++)
        for (size_t j = i + 1; j < g.nodes.size(); j++) {
            double dx = g.nodes[i].x - g.nodes[j].x, dy = g.nodes[i].y - g.nodes[j].y;
            g.edges.push_back({static_cast<int>(i), static_cast<int>(j), std::round(std::hypot(dx, dy) * scale)});
        }
}

Graph random_graph(int n, double density, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> U(0.06, 0.94);
    Graph g;
    g.title = "Random graph (" + std::to_string(n) + " nodes)";
    for (int i = 0; i < n; i++) {
        Node nd;
        nd.name = std::string(1, static_cast<char>('A' + i % 26)) + (i >= 26 ? std::to_string(i / 26) : "");
        // keep nodes apart so labels stay readable
        for (int tries = 0; tries < 200; tries++) {
            nd.x = U(rng); nd.y = U(rng);
            bool ok = true;
            for (const Node& o : g.nodes) if (std::hypot(o.x - nd.x, o.y - nd.y) < 0.12) { ok = false; break; }
            if (ok) break;
        }
        g.nodes.push_back(nd);
    }
    // keep each node's nearest neighbours (planar-looking, readable) + guarantee connectivity
    std::uniform_real_distribution<double> P(0, 1);
    for (int i = 0; i < n; i++) {
        std::vector<std::pair<double, int>> d;
        for (int j = 0; j < n; j++) if (j != i) d.push_back({std::hypot(g.nodes[i].x - g.nodes[j].x, g.nodes[i].y - g.nodes[j].y), j});
        std::sort(d.begin(), d.end());
        int keep = std::max(2, static_cast<int>(density * 6));
        for (int k = 0; k < keep && k < static_cast<int>(d.size()); k++) {
            int j = d[k].second;
            if (k >= 2 && P(rng) > 0.7) continue;
            bool dup = false;
            for (const Edge& e : g.edges) if ((e.u == i && e.v == j) || (e.u == j && e.v == i)) { dup = true; break; }
            if (!dup) g.edges.push_back({std::min(i, j), std::max(i, j), std::round(d[k].first * 100)});
        }
    }
    // join components if any remain
    DSU dsu(n);
    for (const Edge& e : g.edges) dsu.unite(e.u, e.v);
    for (int i = 1; i < n; i++)
        if (dsu.find(i) != dsu.find(0)) {
            double dist = std::hypot(g.nodes[i].x - g.nodes[0].x, g.nodes[i].y - g.nodes[0].y);
            g.edges.push_back({0, i, std::round(dist * 100)});
            dsu.unite(0, i);
        }
    return g;
}

// File format (see data/*.txt):
//   # comment
//   TITLE <words...>
//   NODE <name> <x> <y>          x, y in 0..1 (canvas) - or LATLON <name> <lat> <lon>
//   EDGE <name1> <name2> <weight>  weight may be "auto" = straight-line distance (km for LATLON)
//   COMPLETE <scale>              connect every pair, weight = distance * scale
Graph load_graph(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open " + path);
    Graph g;
    std::unordered_map<std::string, int> idx;
    std::vector<std::pair<double, double>> latlon;
    bool geo = false;
    std::string line;
    int lineno = 0;
    auto km = [&](int a, int b) {
        const double R = 6371.0, p = M_PI / 180.0;
        double la1 = latlon[a].first * p, la2 = latlon[b].first * p, dl = (latlon[b].second - latlon[a].second) * p;
        double h = std::sin((la2 - la1) / 2) * std::sin((la2 - la1) / 2) + std::cos(la1) * std::cos(la2) * std::sin(dl / 2) * std::sin(dl / 2);
        return std::round(2 * R * std::asin(std::sqrt(h)));
    };
    while (std::getline(in, line)) {
        lineno++;
        std::istringstream ss(line);
        std::string kw;
        if (!(ss >> kw) || kw[0] == '#') continue;
        if (kw == "TITLE") { std::getline(ss >> std::ws, g.title); }
        else if (kw == "NODE" || kw == "LATLON") {
            Node n; double a, b;
            if (!(ss >> n.name >> a >> b)) throw std::runtime_error(path + ":" + std::to_string(lineno) + ": bad node");
            if (kw == "LATLON") { geo = true; latlon.push_back({a, b}); n.x = b; n.y = a; }
            else { latlon.push_back({0, 0}); n.x = a; n.y = b; }
            idx[n.name] = static_cast<int>(g.nodes.size());
            g.nodes.push_back(n);
        } else if (kw == "EDGE") {
            std::string a, b, w;
            if (!(ss >> a >> b >> w) || !idx.count(a) || !idx.count(b))
                throw std::runtime_error(path + ":" + std::to_string(lineno) + ": bad edge");
            int u = idx[a], v = idx[b];
            double wt = w == "auto" ? (geo ? km(u, v) : std::round(std::hypot(g.nodes[u].x - g.nodes[v].x, g.nodes[u].y - g.nodes[v].y) * 100)) : std::stod(w);
            g.edges.push_back({u, v, wt});
        } else if (kw == "COMPLETE") {
            double scale = 100; ss >> scale;
            if (geo) {
                g.edges.clear();
                for (size_t i = 0; i < g.nodes.size(); i++)
                    for (size_t j = i + 1; j < g.nodes.size(); j++) g.edges.push_back({(int)i, (int)j, km((int)i, (int)j)});
            } else connect_complete(g, scale);
        } else throw std::runtime_error(path + ":" + std::to_string(lineno) + ": unknown keyword " + kw);
    }
    if (geo) {   // normalise lon/lat into the 0..1 canvas, keeping the aspect ratio
        double x0 = 1e9, x1 = -1e9, y0 = 1e9, y1 = -1e9;
        for (auto& n : g.nodes) { x0 = std::min(x0, n.x); x1 = std::max(x1, n.x); y0 = std::min(y0, n.y); y1 = std::max(y1, n.y); }
        double span = std::max(x1 - x0, y1 - y0);
        for (auto& n : g.nodes) {
            n.x = 0.08 + 0.84 * (n.x - x0) / span + 0.42 * (1 - (x1 - x0) / span);
            n.y = 0.08 + 0.84 * (n.y - y0) / span + 0.42 * (1 - (y1 - y0) / span);
        }
    }
    return g;
}
