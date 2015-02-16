CXX = clang++
CXXFLAGS = -g -std=c++11 -Ofast
LDFLAGS = 

#CXXFILES = $(wildcard tree/*.cc)
#OBJFILES = $(addprefix build/,$(notdir $(CXXFILES:.cc=.o)))

.PHONY: clean

main: $(OBJFILES) build/main.o
	$(CXX) $(LDFLAGS) -o $@ $^

build/main.o: main.cc
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c -o $@ $<

#build/%.o: tree/%.cc
#	mkdir -p build
#	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf build
	rm -f main

