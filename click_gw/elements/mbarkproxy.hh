#ifndef CLICK_MBARKPROXY_HH
#define CLICK_MBARKPROXY_HH
#include <click/ip6address.hh>
#include <click/ipaddress.hh>
#include <click/vector.hh>
#include <click/element.hh>
#include <click/fromfile.hh>

CLICK_DECLS

class MBArkProxy : public Element {

 public:
  MBArkProxy();
  ~MBArkProxy();

  int configure(Vector<String> &, ErrorHandler *);

  const char *class_name() const		{ return "MBArkProxy"; }
  const char *port_count() const		{ return "1/1-2"; }
  void push(int port, Packet *p);

private:
  bool parse_http_req(const char *data, int len, String& url);
};

CLICK_ENDDECLS
#endif
