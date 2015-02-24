CXX = clang++
CXXFLAGS = -g -std=c++11 -Ofast
LDFLAGS = 

#CXXFILES = $(wildcard tree/*.cc)
#OBJFILES = $(addprefix build/,$(notdir $(CXXFILES:.cc=.o)))

.PHONY: clean

main: $(OBJFILES) build/main.o
	$(CXX) $(LDFLAGS) -o $@ $^

click: mbarkgw.cc
	cp -r ../click /tmp/
	cp -r tree /tmp/click/elements/local/
	cp -r mbarkgw.* /tmp/click/elements/local/
	cp -r ope_tree.h /tmp/click/elements/local/
	cp *.click /tmp/click/conf/
	cp *.rules /tmp/click/conf/
	cd /tmp/click; ./configure --enable-nanotimestamp --enable-analysis --enable-ip6 --enable-ipsec --enable-local --enable-multithread --enable-intel-cpu 
	make -C /tmp/click elemlist
	make -C /tmp/click 

build/main.o: main.cc
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c -o $@ $<

#build/%.o: tree/%.cc
#	mkdir -p build
#	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf build
	rm -f main

