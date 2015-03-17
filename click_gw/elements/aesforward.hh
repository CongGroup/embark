#ifndef CLICK_AESFORWARD_HH
#define CLICK_AESFORWARD_HH
#include <click/ip6address.hh>
#include <click/ipaddress.hh>
#include <click/vector.hh>
#include <click/element.hh>
#include <click/fromfile.hh>

CLICK_DECLS

class AESForward : public Element {

 public:
  AESForward();
  ~AESForward();

  int configure(Vector<String> &, ErrorHandler *);

  const char *class_name() const		{ return "AESForward"; }
  const char *port_count() const		{ return "1/1"; }
  Packet *simple_action(Packet *p_in);

  private:
    struct in_addr _saddr;
    struct in_addr _daddr;
    uint16_t _sport;
    uint16_t _dport;
    bool _cksum;
    bool _use_dst_anno;
#if HAVE_FAST_CHECKSUM && FAST_CHECKSUM_ALIGNED
    bool _aligned;
    bool _checked_aligned;
#endif
    atomic_uint32_t _id;
};

CLICK_ENDDECLS
#endif
