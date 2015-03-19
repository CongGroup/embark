#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/args.hh>
#include <click/ip6flowid.hh>
#include <clicknet/ip.h>
#include <clicknet/ip6.h>
#include <clicknet/icmp.h>
#include <clicknet/icmp6.h>
#include <clicknet/tcp.h>
#include <clicknet/udp.h>
#include <openssl/evp.h>

#include "mbarkfwrev.hh"
#include "mbarkfw.hh"

CLICK_DECLS

MBArkFirewallRev::MBArkFirewallRev()
{
}


MBArkFirewallRev::~MBArkFirewallRev()
{
}

int
MBArkFirewallRev::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if (Args(conf, this, errh)
  .read_m("TABLE", ElementCastArg("MBarkTable"), mbarkt_)
  .read("V4", BoolArg(), v4_)
  .complete() < 0)
    return -1;
  return 0;
}

Packet *
MBArkFirewallRev::simple_action(Packet *p_in)
{
  WritablePacket *q = p_in->uniqueify();
  if (v4_) 
  {
    click_ip *ip = (click_ip *) q->data();
    click_udp *udp = (click_udp *) (ip+1);

    q->set_network_header((const unsigned char *) ip, ip->ip_hl * 4);

    IPFlowID after(q, true);
    IPFlowID before = mbarkt_->lookup(after);

    ip->ip_src = before.daddr();
    ip->ip_dst = before.saddr();
    udp->uh_sport = before.dport();
    udp->uh_dport = before.sport();
  }
  else
  {
    click_ip6 *ip = (click_ip6 *)q->data();
    click_udp *udp = (click_udp *)(ip + 1);

    q->set_network_header((const unsigned char *) ip, sizeof(click_ip6));

    IP6FlowID after(q, true);
    IP6FlowID before = mbarkt_->lookup(after);

    IP6Address saddr = before.daddr(), daddr = before.saddr();
    udp->uh_sport = before.dport();
    udp->uh_dport = before.sport();

    memcpy(&(ip->ip6_src), saddr.data(), sizeof(struct in6_addr));
    memcpy(&(ip->ip6_dst), daddr.data(), sizeof(struct in6_addr));
  }
  return q;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MBArkFirewallRev)
ELEMENT_LIBS(-lssl -lcrypto)
