CXX = clang++
CXXFLAGS = -g -std=c++11 -Ofast
LDFLAGS = 

BUILD_DIR = /tmp

RTE_TARGET := x86_64-default-linuxapp-gcc
RTE_SDK := /home/clan/DPDK-1.5.0
export RTE_TARGET
export RTE_SDK

CPPFLAGS := -std=c++11 -D__STDC_LIMIT_MACROS -DUSE_DPDK -I${RTE_SDK}/${RTE_TARGET}/include -L${RTE_SDK}/${RTE_TARGET}/lib
export CPPFLAGS
 
LDFLAGS := -L${RTE_SDK}/${RTE_TARGET}/lib
export LDFLAGS

LIBS := -lethdev -lrte_kni -lrte_malloc -lrte_mempool -lrte_pmd_e1000 -lrte_pmd_ring -lrte_power -lrte_sched -lrte_cmdline -lrte_hash -lrte_lpm  -lrte_mbuf -lrte_meter -lrte_pmd_ixgbe -lrte_pmd_virtio -lrte_ring -lrte_timer -lrte_eal
export LIBS

.PHONY: clean

main: $(OBJFILES) build/main.o
	$(CXX) $(LDFLAGS) -o $@ $^

dpdk: elements/mbarkfw.cc
	cp -r ../click-dpdk $(BUILD_DIR)/
	cp -r elements/* $(BUILD_DIR)/click-dpdk/elements/local/
	cp conf/*.click $(BUILD_DIR)/click-dpdk/conf/
	cp *.rules $(BUILD_DIR)/click-dpdk/conf/
	cp *.sh $(BUILD_DIR)/click-dpdk/
	cd $(BUILD_DIR)/click-dpdk; ./configure --enable-experimental --enable-ip6 --enable-local --disable-linuxmodule --enable-user-multithread --disable-dynamic-linking CPPFLAGS="-Wno-write-strings -Wno-literal-suffix -D__STDC_LIMIT_MACROS -I/home/justine/DPDK-1.5.0/x86_64-default-linuxapp-gcc/include" LDFLAGS="-lcrypto -lssl -L/home/justine/DPDK-1.5.0/x86_64-default-linuxapp-gcc/lib"
#	./configure --disable-linuxmodule --enable-experimental --enable-user-multithread --disable-dynamic-linking --enable-ip6 --enable-ipsec --enable-local --enable-nanotimestamp --enable-intel-cpu CPPFLAGS="-I${RTE_SDK}/${RTE_TARGET}/include -Wno-literal-suffix" LDFLAGS="-L${RTE_SDK}/${RTE_TARGET}/lib" LIBS="${LIBS}"
	make -j 16 -C $(BUILD_DIR)/click-dpdk elemlist
	make -j 16 -C $(BUILD_DIR)/click-dpdk

click: elements/mbarkfw.cc
	cp -r ../click $(BUILD_DIR)/
	cp -r elements/* $(BUILD_DIR)/click/elements/local/
	cp conf/*.click $(BUILD_DIR)/click/conf/
	cp *.rules $(BUILD_DIR)/click/conf/
	cp *.sh $(BUILD_DIR)/click/
	cd $(BUILD_DIR)/click; ./configure --enable-nanotimestamp --enable-analysis --enable-ip6 --enable-ipsec --enable-local --enable-multithread --enable-intel-cpu --enable-experimental
	make -j 16 -C $(BUILD_DIR)/click elemlist
	make -j 16 -C $(BUILD_DIR)/click 

build/main.o: main.cc
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c -o $@ $<

#build/%.o: tree/%.cc
#	mkdir -p build
#	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf build
	rm -f main

