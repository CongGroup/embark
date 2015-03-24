#include "elements/tree/utils.h"
#include "elements/ope_tree.h"
#include <ostream>
#include <fstream>
#include <sstream>
#include <iterator>

static inline uint64_t rdtsc() 
{
    uint64_t d;
    __asm__ __volatile__ ("rdtsc": "=A" (d));
    return d;
}

#ifdef __MACH__
#include <sys/time.h>
#define CLOCK_MONOTONIC                 1
//clock_gettime is not implemented on OSX
int clock_gettime(int /*clk_id*/, struct timespec* t) {
    struct timeval now;
    int rv = gettimeofday(&now, NULL);
    if (rv) return rv;
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
#endif

std::ostream&
operator<<( std::ostream& dest, uint128_t value )
{
    std::ostream::sentry s( dest );
    if ( s ) {
        uint128_t tmp = value;
        char buffer[ 128 ];
        char* d = std::end(buffer);
        do
        {
            -- d;
            *d = "0123456789"[ tmp % 10 ];
            tmp /= 10;
        } while ( tmp != 0 );
        int len = std::end( buffer ) - d;
        if ( dest.rdbuf()->sputn( d, len ) != len ) {
            dest.setstate( std::ios_base::badbit );
        }
    }
    return dest;
}

int test()
{
    OPETree1<uint64_t> tree;
    auto range = tree.ciphertext_range(324);
    std::cout << (range.first) << " " << (range.second) << std::endl;

    std::cout << tree.generate_ciphertext(324) << std::endl;

    tree.add(832423);
    tree.add(23948689);
    tree.add(8819290436560);

    assert(tree.size() == 3);

    auto range1 = tree.ciphertext_range(324);
    auto range2 = tree.ciphertext_range(832874);
    auto range3 = tree.ciphertext_range(881929043656);
    auto range4 = tree.ciphertext_range(8819290436565);

    std::cout << (range1.first) << " " << (range1.second) << std::endl;
    std::cout << range2.first << " " << range2.second << std::endl;
    std::cout << range3.first << " " << range3.second << std::endl;
    std::cout << range4.first << " " << range4.second << std::endl;

    for (int i = 0; i < 100; ++i) {
        std::cout << tree.generate_ciphertext(324) << std::endl;
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <filename> <line>" << std::endl;
        exit(1);
    }

    int n = atoi(argv[2]);

    OPETree1<uint128_t> src_addr_tree_;
    OPETree1<uint128_t> dst_addr_tree_;
    OPETree1<uint32_t> src_addr_tree_v4_;
    OPETree1<uint32_t> dst_addr_tree_v4_;

    OPETree1<uint16_t> src_port_tree_;
    OPETree1<uint16_t> dst_port_tree_;

    OPETree1<uint8_t> proto_tree_;

    std::ifstream infile(argv[1]);
    std::string line;

    int i = 0;
    while (std::getline(infile, line) && (i++ < n))
    {
    //click_chatter("Loading: %s", line.c_str());
        std::string component[6];
        int index = 0, start = 0;
        for (int i = 0; i < line.length(); ++i) 
        {
          if (line[i] == '\t')
          {
            component[index++] = line.substr(start, i-start);
            start = i+1;
          }
        }
        // ip src
        {
          int delim = component[0].find_first_of('/');
          std::string prefix(component[0].substr(1, delim-1));
          int mask_len = atoi(component[0].substr(delim+1).c_str());
          //click_chatter("prefix: %s, len: %d", prefix.c_str(), mask_len);
          uint32_t umask = 0;
          if (mask_len > 0)
              umask = 0xFFFFFFFFU << (32 - mask_len);

          std::istringstream s(prefix);
          char dot;
          int byte1, byte2, byte3, byte4;
          s >> byte1 >> dot >> byte2 >> dot >> byte3 >> dot >> byte4 >> dot;
          uint32_t addr = (byte1 << 24) + (byte2 << 16) + (byte3 << 8) + byte4;

          uint32_t start = addr & umask, end = addr | (~umask);
          src_addr_tree_v4_.add(start);
          src_addr_tree_v4_.add(end);
        }
        // ip dst
        {
          int delim = component[1].find_first_of('/');
          std::string prefix(component[1].substr(1, delim-1));
          int mask_len = atoi(component[1].substr(delim+1).c_str());
          //click_chatter("prefix: %s, len: %d", prefix.c_str(), mask_len);
          uint32_t umask = 0;
          if (mask_len > 0)
              umask = 0xFFFFFFFFU << (32 - mask_len);

          std::istringstream s(prefix);
          char dot;
          int byte1, byte2, byte3, byte4;
          s >> byte1 >> dot >> byte2 >> dot >> byte3 >> dot >> byte4 >> dot;
          uint32_t addr = (byte1 << 24) + (byte2 << 16) + (byte3 << 8) + byte4;

          uint32_t start = addr & umask, end = addr | (~umask);
          dst_addr_tree_v4_.add(start);
          dst_addr_tree_v4_.add(end);
        }
        // port src
        {
          int delim = component[2].find_first_of(':');
          std::string start = component[2].substr(0, delim);
          std::string end = component[2].substr(delim+1);
          src_port_tree_.add(atoi(start.c_str()));
          src_port_tree_.add(atoi(end.c_str()));
        }
        // port dst
        {
          int delim = component[3].find_first_of(':');
          std::string start = component[3].substr(0, delim);
          std::string end = component[3].substr(delim+1);
          dst_port_tree_.add(atoi(start.c_str()));
          dst_port_tree_.add(atoi(end.c_str()));
        }
        // proto
        {
          int delim = component[4].find_first_of(':');
          std::string val = component[4].substr(0, delim);
          std::string mask = component[4].substr(delim+1);
          uint8_t proto = strtoul(val.c_str(), nullptr, 16) & strtoul(mask.c_str(), nullptr, 16);
          proto_tree_.add(proto);
        }
    }
    std::cout << "Finished loading firewall rules. Total: " << i << std::endl;

    srand (time(NULL));
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < 10000; ++i) 
    {
        uint32_t src_addr = rand();
        uint32_t dst_addr = rand();
        uint16_t src_port = rand() % 65536;
        uint16_t dst_port = rand() % 65536;
        uint8_t proto = rand() % 256;

        src_addr_tree_v4_.generate_ciphertext(src_addr);
        dst_addr_tree_v4_.generate_ciphertext(dst_addr);
        src_port_tree_.generate_ciphertext(src_port);
        dst_port_tree_.generate_ciphertext(dst_port);
        proto_tree_.generate_ciphertext(proto);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    std::cout << "Time: " << (1000000000L * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec) / 10000 << " nsec" << std::endl;

    return 0;
}