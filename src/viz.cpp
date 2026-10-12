// kruskal_viz — interactive OpenGL (freeglut) visualisation of Kruskal's algorithm.
//
//   ./kruskal_viz data/india_fibre_network.txt
//   ./kruskal_viz                       (starts with a random graph)
//   ./kruskal_viz data/clrs_example.txt --screenshot out.ppm --step 6   (render one frame to a file, then exit)
//
// Controls: SPACE play/pause · RIGHT/N next step · LEFT/B step back · R reset · E run to end
//           G new random graph · +/- speed · left-click add a city · right-click menu · Q/ESC quit
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "kruskal.hpp"

namespace {

int W = 1180, H = 720;
const int PANEL = 380;                  // right-hand info panel width
Graph G;
KruskalResult R;
int step = 0;                           // number of steps applied (0..R.steps.size())
bool playing = false;
int delay_ms = 700;
unsigned seed = 7;
std::string screenshot_path;
int screenshot_step = -1;

struct RGB { float r, g, b; };
const RGB BG{0.09f, 0.10f, 0.13f}, PANEL_BG{0.13f, 0.14f, 0.18f}, EDGE_IDLE{0.38f, 0.40f, 0.46f},
          ACCEPT{0.30f, 0.82f, 0.47f}, REJECT{0.93f, 0.33f, 0.33f}, CURRENT{1.0f, 0.82f, 0.25f},
          TEXT{0.92f, 0.93f, 0.95f}, MUTED{0.62f, 0.65f, 0.72f};
// one colour per tree, so you can watch union-find merge them
const RGB PALETTE[] = {{0.40f, 0.62f, 1.0f}, {0.98f, 0.55f, 0.35f}, {0.72f, 0.52f, 0.98f}, {0.35f, 0.85f, 0.85f},
                       {0.98f, 0.45f, 0.70f}, {0.78f, 0.85f, 0.35f}, {0.98f, 0.75f, 0.35f}, {0.55f, 0.75f, 0.55f},
                       {0.85f, 0.60f, 0.45f}, {0.60f, 0.60f, 0.95f}, {0.45f, 0.80f, 0.65f}, {0.90f, 0.50f, 0.50f}};

void color(RGB c, float a = 1.f) { glColor4f(c.r, c.g, c.b, a); }

float cx(double x) { return static_cast<float>(40 + x * (W - PANEL - 80)); }
float cy(double y) { return static_cast<float>(40 + y * (H - 80)); }

void text(float x, float y, const std::string& s, RGB c, void* font = GLUT_BITMAP_HELVETICA_12) {
    color(c);
    glRasterPos2f(x, y);
    for (char ch : s) glutBitmapCharacter(font, ch);
}
int text_width(const std::string& s, void* font = GLUT_BITMAP_HELVETICA_12) {
    int w = 0;
    for (char ch : s) w += glutBitmapWidth(font, ch);
    return w;
}

void rebuild() {
    R = kruskal(G);
    step = 0;
    playing = false;
}

void new_random() {
    G = random_graph(12, 0.5, seed++);
    rebuild();
}

// ------------------------------------------------------------------ drawing
void draw_circle(float x, float y, float r, RGB fill) {
    color(fill);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 32; i++) glVertex2f(x + r * std::cos(i * 2 * M_PI / 32), y + r * std::sin(i * 2 * M_PI / 32));
    glEnd();
    color(BG);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 32; i++) glVertex2f(x + r * std::cos(i * 2 * M_PI / 32), y + r * std::sin(i * 2 * M_PI / 32));
    glEnd();
}

int edge_state(int ei, bool& current) {   // 0 idle, 1 accepted, 2 rejected
    current = false;
    const Edge& e = G.edges[ei];
    for (int s = 0; s < step; s++) {
        const Edge& se = R.steps[s].edge;
        if (se.u == e.u && se.v == e.v && se.w == e.w) {
            current = s == step - 1;
            return R.steps[s].kind == StepKind::Accept ? 1 : 2;
        }
    }
    return 0;
}

void draw_graph() {
    // edges: idle first, then rejected, then accepted, then the current one on top
    for (int pass = 0; pass < 4; pass++)
        for (size_t i = 0; i < G.edges.size(); i++) {
            bool cur;
            int st = edge_state(static_cast<int>(i), cur);
            int want = cur ? 3 : st;
            if (want != pass) continue;
            const Edge& e = G.edges[i];
            float x1 = cx(G.nodes[e.u].x), y1 = cy(G.nodes[e.u].y), x2 = cx(G.nodes[e.v].x), y2 = cy(G.nodes[e.v].y);
            RGB c = pass == 0 ? EDGE_IDLE : pass == 1 ? ACCEPT : pass == 2 ? REJECT : CURRENT;
            if (pass == 3) c = R.steps[step - 1].kind == StepKind::Accept ? CURRENT : REJECT;
            glLineWidth(pass == 0 ? 1.5f : pass == 2 ? 1.5f : 4.5f);
            if (pass == 2) { glEnable(GL_LINE_STIPPLE); glLineStipple(2, 0x00FF); }
            color(c, pass == 2 ? 0.8f : 1.f);
            glBegin(GL_LINES); glVertex2f(x1, y1); glVertex2f(x2, y2); glEnd();
            glDisable(GL_LINE_STIPPLE);
            // weight label on a small dark tag
            std::string w = fmt_weight(e.w);
            float mx = (x1 + x2) / 2, my = (y1 + y2) / 2;
            int tw = text_width(w);
            color(BG, 0.85f);
            glRectf(mx - tw / 2.f - 3, my - 5, mx + tw / 2.f + 3, my + 11);
            text(mx - tw / 2.f, my, w, pass == 0 ? MUTED : c);
        }
    // nodes coloured by the tree (union-find set) they currently belong to
    std::vector<int> comp(G.nodes.size());
    for (size_t i = 0; i < comp.size(); i++) comp[i] = step ? R.steps[step - 1].comp[i] : static_cast<int>(i);
    for (size_t i = 0; i < G.nodes.size(); i++) {
        float x = cx(G.nodes[i].x), y = cy(G.nodes[i].y);
        draw_circle(x, y, 9, PALETTE[comp[i] % 12]);
        text(x + 13, y + 8, G.nodes[i].name, TEXT, GLUT_BITMAP_HELVETICA_12);
    }
}

void draw_panel() {
    float x0 = static_cast<float>(W - PANEL);
    color(PANEL_BG);
    glRectf(x0, 0, static_cast<float>(W), static_cast<float>(H));
    float x = x0 + 20, y = static_cast<float>(H - 36);
    text(x, y, "Kruskal's Minimum Spanning Tree", TEXT, GLUT_BITMAP_HELVETICA_18);
    y -= 22;
    text(x, y, G.title.empty() ? "Custom graph" : G.title.substr(0, 52), MUTED);
    y -= 30;
    int N = static_cast<int>(R.steps.size());
    double total = step ? R.steps[step - 1].total : 0;
    int trees = step ? R.steps[step - 1].components : static_cast<int>(G.nodes.size());
    int accepted = 0;
    for (int s = 0; s < step; s++) accepted += R.steps[s].kind == StepKind::Accept;
    text(x, y, "Step " + std::to_string(step) + " of " + std::to_string(N) + (playing ? "   (playing)" : ""), TEXT);
    y -= 20;
    text(x, y, "Tree cost so far: " + fmt_weight(total), ACCEPT, GLUT_BITMAP_HELVETICA_18);
    y -= 20;
    text(x, y, "Edges in tree: " + std::to_string(accepted) + " / " + std::to_string(std::max<int>(0, (int)G.nodes.size() - 1)) +
                    "    Separate trees: " + std::to_string(trees), TEXT);
    y -= 28;
    if (step) {
        const Step& s = R.steps[step - 1];
        std::string e = G.nodes[s.edge.u].name + " - " + G.nodes[s.edge.v].name + " (" + fmt_weight(s.edge.w) + ")";
        text(x, y, "Now: " + e, CURRENT);
        y -= 18;
        text(x, y, s.kind == StepKind::Accept ? "Accepted: joins two different trees" : "Rejected: both ends already in one tree (cycle)",
             s.kind == StepKind::Accept ? ACCEPT : REJECT);
    } else {
        text(x, y, "Edges are taken cheapest first.", MUTED);
        y -= 18;
        text(x, y, "Press SPACE to play or N to step.", MUTED);
    }
    if (step == N && N) {
        y -= 22;
        text(x, y, R.spanning ? "Done: minimum spanning tree = " + fmt_weight(R.total) : "Graph is disconnected: spanning forest only",
             R.spanning ? ACCEPT : REJECT);
    }
    // recent decisions
    y -= 30;
    text(x, y, "Sorted edges (decision log)", TEXT);
    y -= 6;
    int first = std::max(0, step - 12);
    for (int s = first; s < std::min(N, first + 14); s++) {
        y -= 17;
        const Step& st = R.steps[s];
        std::string row = std::to_string(s + 1) + ".  " + G.nodes[st.edge.u].name + " - " + G.nodes[st.edge.v].name + "   " + fmt_weight(st.edge.w);
        RGB c = s >= step ? MUTED : st.kind == StepKind::Accept ? ACCEPT : REJECT;
        text(x, y, row + (s < step ? (st.kind == StepKind::Accept ? "   accept" : "   cycle") : ""), c);
    }
    // legend + controls
    y = 92;
    glLineWidth(4);
    color(ACCEPT); glBegin(GL_LINES); glVertex2f(x, y + 4); glVertex2f(x + 24, y + 4); glEnd();
    text(x + 30, y, "in tree", TEXT);
    color(REJECT); glBegin(GL_LINES); glVertex2f(x + 100, y + 4); glVertex2f(x + 124, y + 4); glEnd();
    text(x + 130, y, "rejected", TEXT);
    color(CURRENT); glBegin(GL_LINES); glVertex2f(x + 210, y + 4); glVertex2f(x + 234, y + 4); glEnd();
    text(x + 240, y, "current", TEXT);
    text(x, 66, "Node colour = which tree it belongs to (union-find)", MUTED);
    text(x, 44, "SPACE play  N/B step  R reset  E end  G random", MUTED);
    text(x, 26, "+/- speed  click: add city  right-click: menu", MUTED);
}

void save_ppm(const std::string& path) {
    std::vector<unsigned char> px(static_cast<size_t>(W) * H * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return;
    std::fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (int row = H - 1; row >= 0; row--) std::fwrite(&px[static_cast<size_t>(row) * W * 3], 1, static_cast<size_t>(W) * 3, f);
    std::fclose(f);
}

void display() {
    glClearColor(BG.r, BG.g, BG.b, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    draw_graph();
    draw_panel();
    glutSwapBuffers();
    if (!screenshot_path.empty()) {
        glReadBuffer(GL_BACK);
        save_ppm(screenshot_path);
        std::exit(0);
    }
}

void tick(int) {
    if (playing && step < static_cast<int>(R.steps.size())) {
        step++;
        glutPostRedisplay();
        if (step == static_cast<int>(R.steps.size())) playing = false;
    }
    glutTimerFunc(delay_ms, tick, 0);
}

void key(unsigned char k, int, int) {
    switch (k) {
    case ' ': if (step == (int)R.steps.size()) step = 0; playing = !playing; break;
    case 'n': case 'N': if (step < (int)R.steps.size()) step++; playing = false; break;
    case 'b': case 'B': if (step > 0) step--; playing = false; break;
    case 'r': case 'R': step = 0; playing = false; break;
    case 'e': case 'E': step = (int)R.steps.size(); playing = false; break;
    case 'g': case 'G': new_random(); break;
    case '+': case '=': delay_ms = std::max(80, delay_ms - 120); break;
    case '-': delay_ms = std::min(2500, delay_ms + 120); break;
    case 'q': case 'Q': case 27: std::exit(0);
    }
    glutPostRedisplay();
}

void special(int k, int, int) {
    if (k == GLUT_KEY_RIGHT) key('n', 0, 0);
    if (k == GLUT_KEY_LEFT) key('b', 0, 0);
    if (k == GLUT_KEY_HOME) key('r', 0, 0);
    if (k == GLUT_KEY_END) key('e', 0, 0);
}

// Left-click in the canvas adds a node joined to its 3 nearest neighbours
// (Euclidean weights), then the algorithm restarts on the new graph.
void mouse(int button, int state, int mx, int my) {
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN || mx > W - PANEL) return;
    Node n;
    n.x = (mx - 40.0) / (W - PANEL - 80);
    n.y = ((H - my) - 40.0) / (H - 80);
    n.name = "N" + std::to_string(G.nodes.size() + 1);
    int id = static_cast<int>(G.nodes.size());
    std::vector<std::pair<double, int>> d;
    for (int i = 0; i < id; i++) d.push_back({std::hypot(G.nodes[i].x - n.x, G.nodes[i].y - n.y), i});
    std::sort(d.begin(), d.end());
    G.nodes.push_back(n);
    for (size_t k = 0; k < d.size() && k < 3; k++) G.edges.push_back({d[k].second, id, std::round(d[k].first * 100)});
    if (G.title.find("(edited)") == std::string::npos) G.title += " (edited)";
    rebuild();
    glutPostRedisplay();
}

void menu(int option) {   // right-click menu, as in the original project
    const char keys[] = {'n', ' ', 'r', 'e', 'g', 'q'};
    if (option >= 0 && option < 6) key(keys[option], 0, 0);
}

void reshape(int w, int h) {
    W = w; H = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
}

}  // namespace

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    std::string file;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--screenshot" && i + 1 < argc) screenshot_path = argv[++i];
        else if (a == "--step" && i + 1 < argc) screenshot_step = std::atoi(argv[++i]);
        else if (a == "--seed" && i + 1 < argc) seed = static_cast<unsigned>(std::atoi(argv[++i]));
        else file = a;
    }
    try {
        if (file.empty()) new_random();
        else { G = load_graph(file); rebuild(); }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    if (screenshot_step >= 0) step = std::min<int>(screenshot_step, static_cast<int>(R.steps.size()));

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_MULTISAMPLE);
    glutInitWindowSize(W, H);
    glutCreateWindow("Kruskal's Minimum Spanning Tree - OpenGL");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutCreateMenu(menu);
    glutAddMenuEntry("Next step (N)", 0);
    glutAddMenuEntry("Play / pause (SPACE)", 1);
    glutAddMenuEntry("Reset (R)", 2);
    glutAddMenuEntry("Run to end (E)", 3);
    glutAddMenuEntry("New random graph (G)", 4);
    glutAddMenuEntry("Quit (Q)", 5);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    glutTimerFunc(delay_ms, tick, 0);
    glutMainLoop();
    return 0;
}
