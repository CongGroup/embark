CXX := clang++
CXXFLAGS := -std=c++11 -I.. -Wno-deprecated-declarations
LIBS := -lcrypto -lssl -lntl -lgmp

SRC := x.cc ope.cc urandom.cc hgd.cc
OBJ := $(SRC:%.cc=%.o)

all: x

x: $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

# vim: set noexpandtab:
