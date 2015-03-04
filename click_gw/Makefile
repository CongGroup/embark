CXX = clang++
CXXFLAGS = -g -std=c++11 -Ofast
LDFLAGS = 

BUILD_DIR = /tmp

RTE_TARGET := x86_64-default-linuxapp-gcc
RTE_SDK := /home/clan/DPDK-1.5.0
export RTE_TARGET
export RTE_SDK

CPPFLAGS := -D__STDC_LIMIT_MACROS -DUSE_DPDK -I${RTE_SDK}/${RTE_TARGET}/include -std=gnu++11 -L${RTE_SDK}/${RTE_TARGET}/lib
export CPPFLAGS
 
LDFLAGS := -L${RTE_SDK}/${RTE_TARGET}/lib
export LDFLAGS

LIBS := -lethdev -lrte_kni -lrte_malloc -lrte_mempool -lrte_pmd_e1000 -lrte_pmd_ring -lrte_power -lrte_sched -lrte_cmdline -lrte_hash -lrte_lpm  -lrte_mbuf -lrte_meter -lrte_pmd_ixgbe -lrte_pmd_virtio -lrte_ring -lrte_timer -lrte_eal
export LIBS

#CXXFILES = $(wildcard tree/*.cc)
#OBJFILES = $(addprefix build/,$(notdir $(CXXFILES:.cc=.o)))

.PHONY: clean

main: $(OBJFILES) build/main.o
	$(CXX) $(LDFLAGS) -o $@ $^

dpdk: mbarkgw.cc
	cp -r ../dpdk-click $(BUILD_DIR)/
	cp -r tree $(BUILD_DIR)/dpdk-click/elements/local/
	cp -r mbarkgw.* $(BUILD_DIR)/dpdk-click/elements/local/
	cp -r ope_tree.h $(BUILD_DIR)/dpdk-click/elements/local/
	cp *.click $(BUILD_DIR)/dpdk-click/conf/
	cp *.rules $(BUILD_DIR)/dpdk-click/conf/
	cp *.sh $(BUILD_DIR)/dpdk-click/
	cd $(BUILD_DIR)/dpdk-click; ./configure --disable-linuxmodule --enable-experimental --enable-user-multithread --disable-dynamic-linking CPPFLAGS="-I${RTE_SDK}/${RTE_TARGET}/include" LDFLAGS="-L${RTE_SDK}/${RTE_TARGET}/lib" LIBS="${LIBS}"
	#make -C $(BUILD_DIR)/dpdk-click elemlist
	make -C $(BUILD_DIR)/dpdk-click

click: mbarkgw.cc
	cp -r ../click $(BUILD_DIR)/
	cp -r tree $(BUILD_DIR)/click/elements/local/
	cp -r mbarkgw.* $(BUILD_DIR)/click/elements/local/
	cp -r ope_tree.h $(BUILD_DIR)/click/elements/local/
	cp *.click $(BUILD_DIR)/click/conf/
	cp *.rules $(BUILD_DIR)/click/conf/
	cp *.sh $(BUILD_DIR)/click/
	cd $(BUILD_DIR)/click; ./configure --enable-nanotimestamp --enable-analysis --enable-ip6 --enable-ipsec --enable-local --enable-multithread --enable-intel-cpu 
	make -C $(BUILD_DIR)/click elemlist
	make -C $(BUILD_DIR)/click 

build/main.o: main.cc
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c -o $@ $<

#build/%.o: tree/%.cc
#	mkdir -p build
#	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf build
	rm -f main

