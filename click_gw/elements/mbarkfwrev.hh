#ifndef CLICK_MBARKGWREV_HH
#define CLICK_MBARKGWREV_HH
#include <click/ip6address.hh>
#include <click/ipaddress.hh>
#include <click/vector.hh>
#include <click/element.hh>
#include <click/fromfile.hh>

#include "mbarktable.hh"

CLICK_DECLS

class MBArkFirewallRev : public Element {

 public:
  MBArkFirewallRev();
  ~MBArkFirewallRev();

  int configure(Vector<String> &, ErrorHandler *);

  const char *class_name() const		{ return "MBArkFirewallRev"; }
  const char *port_count() const		{ return PORTS_1_1; }
  Packet *simple_action(Packet *p_in);

private:
  bool v4_;
  MBarkTable *mbarkt_;
};

CLICK_ENDDECLS
#endif
