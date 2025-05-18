CXX = g++
CXXFLAGS = -std=c++17 -Wall -I/opt/homebrew/include -I$(HOME)/ns-3-dev/build/include
LDFLAGS = -L/opt/homebrew/lib -lzmq -pthread

# Simple version that doesn't depend on ns-3 libraries
all: cross_layer_sim

cross_layer_sim: cross_layer_sim.cc
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f cross_layer_sim

.PHONY: clean all
