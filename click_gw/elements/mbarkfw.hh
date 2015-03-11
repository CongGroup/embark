#ifndef CLICK_MBARKGW_HH
#define CLICK_MBARKGW_HH
#include <click/ip6address.hh>
#include <click/ipaddress.hh>
#include <click/vector.hh>
#include <click/element.hh>
#include <click/fromfile.hh>

#include "ope_tree.h"

struct ext_hdr {
  uint8_t next_hdr;
  uint8_t hdr_ext_len;
  unsigned char __padding[6];
  unsigned char ciphertext[48];
};

CLICK_DECLS

class MBArkFirewall : public Element {

 public:
  MBArkFirewall();
  ~MBArkFirewall();

  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);

//  MBArkFirewall *hotswap_element() const;
//  void take_state(Element *, ErrorHandler *);

  const char *class_name() const		{ return "MBArkFirewall"; }
  const char *port_count() const		{ return PORTS_1_1; }
  void push(int port, Packet *p);
  void encrypt(Packet *);

private:
  OPETree1<uint128_t> src_addr_tree_;
  OPETree1<uint128_t> dst_addr_tree_;
  OPETree1<uint16_t> src_port_tree_;
  OPETree1<uint16_t> dst_port_tree_;
  OPETree1<uint8_t> proto_tree_;

  FromFile _ff;
};

CLICK_ENDDECLS
#endif
