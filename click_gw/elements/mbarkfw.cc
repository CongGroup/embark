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

#include "mbarkfw.hh"

CLICK_DECLS

MBArkFirewall::MBArkFirewall()
: v4_(false), mbarkt_(nullptr), enable_(true), stateful_(true)
{
}


MBArkFirewall::~MBArkFirewall()
{
}

int
MBArkFirewall::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if (ff_.configure_keywords(conf, this, errh) < 0)
    return -1;
  if (Args(conf, this, errh)
  .read_m("FILENAME", FilenameArg(), ff_.filename())
  .read_m("TABLE", ElementCastArg("MBarkTable"), mbarkt_)
  .read("V4", BoolArg(), v4_)
  .read("ENABLE", BoolArg(), enable_)
  .read("STATEFUL", BoolArg(), stateful_)
  .complete() < 0)
    return -1;
  return 0;
}

int
MBArkFirewall::initialize(ErrorHandler *errh)
{
  if (ff_.initialize(errh) < 0)
    return -1;
  String line;
  while (ff_.read_line(line, errh) > 0)
  {
    //click_chatter("Loading: %s", line.c_str());
    String component[6];
    int index = 0, start = 0;
    for (int i = 0; i < line.length(); ++i) 
    {
      if (line[i] == '\t')
      {
        component[index++] = line.substring(start, i-start);
        start = i+1;
      }
    }
    // ip src
    {
      int delim = component[0].find_left('/');
      String prefix(component[0].substring(1, delim-1));
      int mask_len = atoi(component[0].substring(delim+1).c_str());
      //click_chatter("prefix: %s, len: %d", prefix.c_str(), mask_len);
      IPAddress mask(IPAddress::make_prefix(mask_len));
      IPAddress addr(IPAddress(prefix) & mask);
      if (v4_)
      {
        IPAddress start = addr, end = addr | (~mask);
        src_addr_tree_v4_.add(ntohl(start.addr()));
        src_addr_tree_v4_.add(ntohl(end.addr()));
      }
      else 
      {
        IP6Address start(addr);
        IP6Address end(addr | (~mask));
        //click_chatter("start: %s, end: %s", start.s().c_str(), end.s().c_str());

        uint128_t *p1 = (uint128_t *) start.data();
        src_addr_tree_.add(ntoh128(*p1));
        uint128_t *p2 = (uint128_t *) end.data();
        src_addr_tree_.add(ntoh128(*p2));
        //assert(ntoh128(*p1) <= ntoh128(*p2));
      }
    }
    // ip dst
    {
      int delim = component[1].find_left('/');
      String prefix(component[1].substring(0, delim));
      int mask_len = atoi(component[1].substring(delim+1).c_str());
      //click_chatter("prefix: %s, len: %d", prefix.c_str(), mask_len);
      IPAddress mask(IPAddress::make_prefix(mask_len));
      IPAddress addr(IPAddress(prefix) & mask);
      if (v4_)
      {
        IPAddress start = addr, end = addr | (~mask);
        dst_addr_tree_v4_.add(ntohl(start.addr()));
        dst_addr_tree_v4_.add(ntohl(end.addr()));
      }
      else 
      {
        IP6Address start(addr);
        IP6Address end(addr | (~mask));
        //click_chatter("start: %s, end: %s", start.s().c_str(), end.s().c_str());

        uint128_t *p = (uint128_t *) start.data();
        dst_addr_tree_.add(ntoh128(*p));
        p = (uint128_t *) end.data();
        dst_addr_tree_.add(ntoh128(*p));
      }
    }
    // port src
    {
      int delim = component[2].find_left(':');
      String start = component[2].substring(0, delim);
      String end = component[2].substring(delim+1);
      src_port_tree_.add(atoi(start.c_str()));
      src_port_tree_.add(atoi(end.c_str()));
    }
    // port dst
    {
      int delim = component[3].find_left(':');
      String start = component[3].substring(0, delim);
      String end = component[3].substring(delim+1);
      dst_port_tree_.add(atoi(start.c_str()));
      dst_port_tree_.add(atoi(end.c_str()));
    }
    // proto
    {
      int delim = component[4].find_left('/');
      String val = component[4].substring(0, delim);
      String mask = component[4].substring(delim+1);
      uint8_t proto = strtoul(val.c_str(), nullptr, 16) & strtoul(mask.c_str(), nullptr, 16);
      proto_tree_.add(proto);
    }
  }
  click_chatter("Finished loading firewall rules.");
  return 0;
}

void
MBArkFirewall::push(int, Packet *p)
{
  (v4_) ? encrypt_v4(p) : encrypt_v6(p);
}

void
MBArkFirewall::encrypt_v4(Packet *p)
{
  WritablePacket *q = p->uniqueify();
  click_ip *ip = (click_ip *)q->data();
  click_udp *udp = (click_udp *)(ip + 1);
  q->set_network_header((const unsigned char *) ip, ip->ip_hl * 4);

  uint32_t *src_addr = (uint32_t *) &(ip->ip_src);
  uint32_t *dst_addr = (uint32_t *) &(ip->ip_dst);
  uint16_t *src_port = &(udp->uh_sport);
  uint16_t *dst_port = &(udp->uh_dport);

  IPFlowID before(q);

  uint32_t cipher_src_addr = htonl(src_addr_tree_v4_.generate_ciphertext(ntohl(*src_addr)));
  uint32_t cipher_dst_addr = htonl(dst_addr_tree_v4_.generate_ciphertext(ntohl(*dst_addr)));
  uint16_t cipher_src_port = htons(src_port_tree_.generate_ciphertext(ntohs(*src_port)));
  uint16_t cipher_dst_port = htons(dst_port_tree_.generate_ciphertext(ntohs(*dst_port)));

  if (enable_)
  {
    memcpy(&(ip->ip_src), &cipher_src_addr, sizeof(uint32_t));
    memcpy(&(ip->ip_dst), &cipher_dst_addr, sizeof(uint32_t));
    memcpy(&(udp->uh_sport), &cipher_src_port, sizeof(uint16_t));
    memcpy(&(udp->uh_dport), &cipher_dst_port, sizeof(uint16_t));
  }

  if (stateful_)
  {
    IPFlowID after(q);
    mbarkt_->insert(before, after);
  }
  else
  {
    EVP_CIPHER_CTX ctx;
    unsigned char key[16] = {0}; // TODO: something fake
    unsigned char iv[16] = {0};
    int outlen = 0, totallen = 0;
    unsigned char cipher[1024];
    EVP_EncryptInit(&ctx, EVP_aes_128_cfb(), key, iv);
    EVP_EncryptUpdate(&ctx, cipher + totallen, &outlen, (unsigned char *) &cipher_src_addr, sizeof(uint32_t));
    totallen += outlen;
    EVP_EncryptUpdate(&ctx, cipher + totallen, &outlen, (unsigned char *) &cipher_dst_addr, sizeof(uint32_t));
    totallen += outlen;
    EVP_EncryptUpdate(&ctx, cipher + totallen, &outlen, (unsigned char *) &cipher_src_port, sizeof(uint16_t));
    totallen += outlen;
    EVP_EncryptUpdate(&ctx, cipher + totallen, &outlen, (unsigned char *) &cipher_dst_port, sizeof(uint16_t));
    totallen += outlen;
    EVP_EncryptFinal(&ctx, cipher + totallen, &outlen);
    totallen += outlen;
  }

  output(0).push(q);
}

void
MBArkFirewall::encrypt_v6(Packet *p)
{
  WritablePacket *q = p->uniqueify();

  click_ip6 *ip = (click_ip6 *)q->data();
  //ext_hdr *option = (ext_hdr *)(ip + 1);

  //q = q->put(sizeof(ext_hdr));
  int plen = ntohs(ip->ip6_plen);

  //memcpy(option + 1, option, plen);
  //memset(option, 0, sizeof(ext_hdr));
  //option->next_hdr = ip->ip6_nxt;
  //option->hdr_ext_len = 6;

  //ip->ip6_nxt = 60;
  //ip->ip6_plen = htons(plen + sizeof(ext_hdr));

  click_udp *udp = (click_udp *)(ip + 1);

  uint128_t *src_addr = (uint128_t *) &(ip->ip6_src);
  uint128_t *dst_addr = (uint128_t *) &(ip->ip6_dst);
  uint16_t *src_port = &(udp->uh_sport);
  uint16_t *dst_port = &(udp->uh_dport);

  q->set_network_header((const unsigned char *) ip, sizeof(click_ip6));
  IP6FlowID before(q);

  uint128_t cipher_src_addr = hton128(src_addr_tree_.generate_ciphertext(ntoh128(*src_addr)));
  uint128_t cipher_dst_addr = hton128(dst_addr_tree_.generate_ciphertext(ntoh128(*dst_addr)));
  uint16_t cipher_src_port = htons(src_port_tree_.generate_ciphertext(ntohs(*src_port)));
  uint16_t cipher_dst_port = htons(dst_port_tree_.generate_ciphertext(ntohs(*dst_port)));

  if (enable_)
  {
    memcpy(&(ip->ip6_src), &cipher_src_addr, sizeof(uint128_t));
    memcpy(&(ip->ip6_dst), &cipher_dst_addr, sizeof(uint128_t));
    memcpy(&(udp->uh_sport), &cipher_src_port, sizeof(uint16_t));
    memcpy(&(udp->uh_dport), &cipher_dst_port, sizeof(uint16_t));
  }

  if (stateful_)
  {
    IP6FlowID after(q);
    mbarkt_->insert(before, after);
  }
  else
  {
    EVP_CIPHER_CTX ctx;
    unsigned char key[16] = {0}; // TODO: something fake
    unsigned char iv[16] = {0};
    int outlen = 0, totallen = 0;
    unsigned char cipher[1024];
    EVP_EncryptInit(&ctx, EVP_aes_128_cfb(), key, iv);
    EVP_EncryptUpdate(&ctx, cipher + totallen, &outlen, (unsigned char *) &cipher_src_addr, sizeof(uint128_t));
    totallen += outlen;
    EVP_EncryptUpdate(&ctx, cipher + totallen, &outlen, (unsigned char *) &cipher_dst_addr, sizeof(uint128_t));
    totallen += outlen;
    EVP_EncryptUpdate(&ctx, cipher + totallen, &outlen, (unsigned char *) &cipher_src_port, sizeof(uint16_t));
    totallen += outlen;
    EVP_EncryptUpdate(&ctx, cipher + totallen, &outlen, (unsigned char *) &cipher_dst_port, sizeof(uint16_t));
    totallen += outlen;
    EVP_EncryptFinal(&ctx, cipher + totallen, &outlen);
    totallen += outlen;
  }

  output(0).push(q);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MBArkFirewall)
ELEMENT_LIBS(-lssl -lcrypto)
