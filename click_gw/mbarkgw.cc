#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include <clicknet/ip6.h>
#include <clicknet/icmp.h>
#include <clicknet/icmp6.h>
#include <clicknet/tcp.h>
#include <clicknet/udp.h>

#include "mbarkgw.hh"

CLICK_DECLS

MBArkGateway::MBArkGateway()
{
}


MBArkGateway::~MBArkGateway()
{
}

void
MBArkGateway::push(int, Packet *p)
{
    encrypt(p);
}


void
MBArkGateway::encrypt(Packet *p)
{
  WritablePacket *q = p->uniqueify();
  click_ip6 *ip = (click_ip6 *)q->data();
  click_udp *udp = (click_udp *)(ip + 1);
  uint128_t *src_addr = (uint128_t *) &(ip->ip6_src);
  uint128_t *dst_addr = (uint128_t *) &(ip->ip6_dst);
  uint16_t *src_port = &(udp->uh_sport);
  uint16_t *dst_port = &(udp->uh_dport);

  uint128_t cipher_src_addr = hton128(src_addr_tree_.generate_ciphertext(ntoh128(*src_addr)));
  uint128_t cipher_dst_addr = hton128(dst_addr_tree_.generate_ciphertext(ntoh128(*dst_addr)));
  uint16_t cipher_src_port = htons(src_port_tree_.generate_ciphertext(ntohs(*src_port)));
  uint16_t cipher_dst_port = htons(dst_port_tree_.generate_ciphertext(ntohs(*dst_port)));

  memcpy(&(ip->ip6_src), &cipher_src_addr, sizeof(uint128_t));
  memcpy(&(ip->ip6_dst), &cipher_dst_addr, sizeof(uint128_t));
  memcpy(&(udp->uh_sport), &cipher_src_port, sizeof(uint16_t));
  memcpy(&(udp->uh_dport), &cipher_dst_port, sizeof(uint16_t));

  output(0).push(q);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MBArkGateway)
