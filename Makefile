CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
GL_LIBS  ?= -lglut -lGLU -lGL
ifeq ($(shell uname),Darwin)
  GL_LIBS = -framework GLUT -framework OpenGL
  CXXFLAGS += -DGL_SILENCE_DEPRECATION
endif

all: kruskal_cli kruskal_viz

kruskal_cli: src/cli.cpp src/kruskal.cpp src/kruskal.hpp
	$(CXX) $(CXXFLAGS) -o $@ src/cli.cpp src/kruskal.cpp

kruskal_viz: src/viz.cpp src/kruskal.cpp src/kruskal.hpp
	$(CXX) $(CXXFLAGS) -o $@ src/viz.cpp src/kruskal.cpp $(GL_LIBS)

test_kruskal: tests/test_kruskal.cpp src/kruskal.cpp src/kruskal.hpp
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined -g -DDATA_DIR='"data"' -o $@ tests/test_kruskal.cpp src/kruskal.cpp

test: test_kruskal
	./test_kruskal

clean:
	rm -f kruskal_cli kruskal_viz test_kruskal

.PHONY: all test clean
