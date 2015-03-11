#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/args.hh>
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
{
}


MBArkFirewall::~MBArkFirewall()
{
}

int
MBArkFirewall::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if (_ff.configure_keywords(conf, this, errh) < 0)
    return -1;
  if (Args(conf, this, errh)
  .read_mp("FILENAME", FilenameArg(), _ff.filename())
  .complete() < 0)
    return -1;
  return 0;
}

int
MBArkFirewall::initialize(ErrorHandler *errh)
{
  if (_ff.initialize(errh) < 0)
    return -1;
  String line;
  while (_ff.read_line(line, errh) > 0)
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
      IP6Address start(addr);
      IP6Address end(addr | (~mask));
      //click_chatter("start: %s, end: %s", start.s().c_str(), end.s().c_str());

      uint128_t *p1 = (uint128_t *) start.data();
      src_addr_tree_.add(ntoh128(*p1));
      uint128_t *p2 = (uint128_t *) end.data();
      src_addr_tree_.add(ntoh128(*p2));
      //assert(ntoh128(*p1) <= ntoh128(*p2));
    }
    // ip dst
    {
      int delim = component[1].find_left('/');
      String prefix(component[1].substring(0, delim));
      int mask_len = atoi(component[1].substring(delim+1).c_str());
      //click_chatter("prefix: %s, len: %d", prefix.c_str(), mask_len);
      IPAddress mask(IPAddress::make_prefix(mask_len));
      IPAddress addr(IPAddress(prefix) & mask);
      IP6Address start(addr);
      IP6Address end(addr | (~mask));
      //click_chatter("start: %s, end: %s", start.s().c_str(), end.s().c_str());

      uint128_t *p = (uint128_t *) start.data();
      dst_addr_tree_.add(ntoh128(*p));
      p = (uint128_t *) end.data();
      dst_addr_tree_.add(ntoh128(*p));
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
    encrypt(p);
}


void
MBArkFirewall::encrypt(Packet *p)
{
  WritablePacket *q = p->uniqueify();

  click_ip6 *ip = (click_ip6 *)q->data();
  ext_hdr *option = (ext_hdr *)(ip + 1);

  q = q->put(sizeof(ext_hdr));
  int plen = ntohs(ip->ip6_plen);

  memcpy(option + 1, option, plen);
  memset(option, 0, sizeof(ext_hdr));
  option->next_hdr = ip->ip6_nxt;
  option->hdr_ext_len = 6;

  ip->ip6_nxt = 60;
  ip->ip6_plen = htons(plen + sizeof(ext_hdr));

  click_udp *udp = (click_udp *)(option + 1);

  uint128_t *src_addr = (uint128_t *) &(ip->ip6_src);
  uint128_t *dst_addr = (uint128_t *) &(ip->ip6_dst);
  uint16_t *src_port = &(udp->uh_sport);
  uint16_t *dst_port = &(udp->uh_dport);

  EVP_CIPHER_CTX ctx;
  unsigned char key[32] = {0}; // TODO: something fake
  unsigned char iv[16] = {0};
  int outlen = 0, totallen = 0;

  EVP_EncryptInit(&ctx, EVP_aes_128_cbc(), key, iv);

  EVP_EncryptUpdate(&ctx, option->ciphertext, &outlen, (unsigned char *) src_addr, sizeof(uint128_t));
  totallen += outlen;
  
  EVP_EncryptUpdate(&ctx, option->ciphertext + totallen, &outlen, (unsigned char *) dst_addr, sizeof(uint128_t));
  totallen += outlen;

  EVP_EncryptUpdate(&ctx, option->ciphertext + totallen, &outlen, (unsigned char *) src_port, sizeof(uint16_t));
  totallen += outlen;

  EVP_EncryptUpdate(&ctx, option->ciphertext + totallen, &outlen, (unsigned char *) dst_port, sizeof(uint16_t));
  totallen += outlen;

  EVP_EncryptFinal(&ctx, option->ciphertext + totallen, &outlen);
  totallen += outlen;

  uint128_t cipher_src_addr = hton128(src_addr_tree_.generate_ciphertext(ntoh128(*src_addr)));
  uint128_t cipher_dst_addr = hton128(dst_addr_tree_.generate_ciphertext(ntoh128(*dst_addr)));
  uint16_t cipher_src_port = htons(src_port_tree_.generate_ciphertext(ntohs(*src_port)));
  uint16_t cipher_dst_port = htons(dst_port_tree_.generate_ciphertext(ntohs(*dst_port)));

  /*
  memcpy(&(ip->ip6_src), &cipher_src_addr, sizeof(uint128_t));
  memcpy(&(ip->ip6_dst), &cipher_dst_addr, sizeof(uint128_t));
  memcpy(&(udp->uh_sport), &cipher_src_port, sizeof(uint16_t));
  memcpy(&(udp->uh_dport), &cipher_dst_port, sizeof(uint16_t));
  */

  output(0).push(q);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MBArkFirewall)
ELEMENT_LIBS(-lssl -lcrypto)
