#include "elements/tree/utils.h"
#include "elements/ope_tree.h"
#include <ostream>
#include <fstream>
#include <sstream>
#include <iterator>

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

bool is_prefix(const std::string& s)
{
  return !(s.find('/') == std::string::npos);
}

bool is_v6(const std::string& s)
{
  return !(s.find(':') == std::string::npos);
}


std::pair<uint128_t, uint128_t> 
decode_prefix(const std::string& s)
{
  int slash_pos = s.find('/');
  std::string addr = s.substr(0, slash_pos);
  int prefix = atoi(s.substr(slash_pos+1).c_str());
  
  if (is_v6(s))
  {
    uint128_t in6;
    inet_pton(AF_INET6, addr.c_str(), &in6);

    uint128_t mask = 0;
    mask = (~mask) << (128 - prefix);

    uint128_t start = ntoh128(in6) & mask;
    uint128_t end = start | (~mask);
    
    return std::make_pair(start, end);
  }
  else
  {
    uint32_t in4;
    inet_pton(AF_INET, addr.c_str(), &in4);

    uint32_t mask = 0;
    mask = (~mask) << (32 - prefix);

    uint32_t start = ntohl(in4) & mask;
    uint32_t end = start | ~(mask);

    uint128_t mask2 = 0xffff;
    mask2 <<= 32;

    uint128_t mapped_start = start | mask2;
    uint128_t mapped_end = end | mask2;

    return std::make_pair(mapped_start, mapped_end);
  }
}

uint128_t 
decode_addr(const std::string& s)
{
  if (is_v6(s))
  {
    uint128_t in6;
    inet_pton(AF_INET6, s.c_str(), &in6);
    return in6;
  }
  else
  {
    uint32_t in4;
    inet_pton(AF_INET, s.c_str(), &in4);
    uint128_t mask2 = 0xffff;
    mask2 <<= 32;
    return (ntohl(in4) | mask2);
  }
}

void handle_addr(const std::string& s, OPETree1<uint128_t>& t)
{
  if (s == "any")
    return;

  if (is_prefix(s)) 
  {
    auto pair = decode_prefix(s);  
    //std::cout << "before add " << pair.first << " " << t.findEQ(pair.first) << std::endl;
    t.add(pair.first);
    //std::cout << "before add " << pair.second << std::endl;
    t.add(pair.second);
    //std::cout << "add" << std::endl;
  }
  else
  {
    auto addr = decode_addr(s);
    //std::cout << "before add " << addr << " " << t.findEQ(addr) << std::endl;
    t.add(addr);
  }
}

void handle_port(const std::string& s, OPETree1<uint16_t>& t)
{
  if (s == "any")
    return;
  t.add(atoi(s.c_str()));
}

void output_addr(const std::string& s, const OPETree1<uint128_t>& t)
{
  if (s == "any")
  {
    std::cout << "any _ ";
    return;
  }

  if (is_prefix(s)) 
  {
    auto pair = decode_prefix(s);
    uint128_t start = hton128(t.ciphertext_of(pair.first));
    uint128_t end = hton128(t.ciphertext_of(pair.second));
    char start_buf[1024], end_buf[1024];
    inet_ntop(AF_INET6, &(start), start_buf, 1024);
    inet_ntop(AF_INET6, &(end), end_buf, 1024);
    std::cout << start_buf << " " << end_buf << " ";
  }
  else
  {
    auto addr = decode_addr(s);
    uint128_t start = hton128(t.ciphertext_of(addr));
    char start_buf[1024];
    inet_ntop(AF_INET6, &(start), start_buf, 1024);
    std::cout << start_buf << " _ ";
  }
}

void output_port(const std::string& s, const OPETree1<uint16_t>& t)
{
  if (s == "any")
  {
    std::cout << "any ";
    return;
  }
  uint16_t p = atoi(s.c_str());
  uint128_t q = t.ciphertext_of(p);
  std::cout << q << " ";
}

void test(const char *filename)
{
    OPETree1<uint128_t> sa_tree, da_tree;
    //OPETree1<uint16_t> sp_tree, dp_tree;

    std::ifstream infile(filename);
    std::string line;

    while (std::getline(infile, line)) 
    {
      std::string sa, da, sp, dp;
      std::istringstream iss(line);
      iss >> sa >> da >> sp >> dp;
      //std::cout << sa << " " << da << " " << sp <<  " " << dp << std::endl;
      handle_addr(sa, sa_tree);
      //handle_addr(da, da_tree);
      //handle_port(sp, sp_tree);
      //handle_port(dp, dp_tree);
    }
    infile.close();
    infile.open(filename);
    while (std::getline(infile, line)) 
    {
      std::string sa, da, sp, dp;
      std::istringstream iss(line);
      iss >> sa >> da >> sp >> dp;
      //std::cout << sa << " " << da << " " << sp <<  " " << dp << std::endl;
      output_addr(sa, sa_tree);
      //output_addr(da, da_tree);
      //output_port(sp, sp_tree);
      //output_port(dp, dp_tree);
      std::cout << std::endl;
    }
}

int main(int argc, char **argv)
{
  test("../fw_rules/bsd.raw");
  return 0;
}
