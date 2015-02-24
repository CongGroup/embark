#ifndef CLICK_MBARKGW_HH
#define CLICK_MBARKGW_HH
#include <click/ip6address.hh>
#include <click/ipaddress.hh>
#include <click/vector.hh>
#include <click/element.hh>
#include <click/fromfile.hh>

#include "ope_tree.h"

CLICK_DECLS

class MBArkGateway : public Element {

 public:
  MBArkGateway();
  ~MBArkGateway();

  int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
  int initialize(ErrorHandler *) CLICK_COLD;

//  MBArkGateway *hotswap_element() const;
//  void take_state(Element *, ErrorHandler *);

  const char *class_name() const		{ return "MBArkGateway"; }
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
