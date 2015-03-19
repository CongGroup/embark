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
#include <cstring>
#include <click/standard/alignmentinfo.hh>
#include "aesforward.hh"


CLICK_DECLS

AESForward::AESForward()
    : _cksum(true), _use_dst_anno(false)
{
    _id = 0;
    _sport = 0;
    _dport = 0;
#if HAVE_FAST_CHECKSUM && FAST_CHECKSUM_ALIGNED
    _checked_aligned = false;
#endif
}

AESForward::~AESForward()
{
}

int
AESForward::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if (Args(conf, this, errh)
  //.read_mp("FILENAME", FilenameArg(), _ff.filename())
  .complete() < 0)
    return -1;
#if HAVE_FAST_CHECKSUM && FAST_CHECKSUM_ALIGNED
    if (!_checked_aligned) {
  int ans, c, o;
  ans = AlignmentInfo::query(this, 0, c, o);
  _aligned = (ans && c == 4 && o == 0);
  if (!_aligned)
      errh->warning("IP header unaligned, cannot use fast IP checksum");
  if (!ans)
      errh->message("(Try passing the configuration through %<click-align%>.)");
  _checked_aligned = true;
    }
#endif
    return 0;
}


Packet *
AESForward::simple_action(Packet *p_in)
{
  WritablePacket *p = p_in->push(sizeof(click_udp) + sizeof(click_ip));
  click_ip *ip = reinterpret_cast<click_ip *>(p->data());
  click_udp *udp = reinterpret_cast<click_udp *>(ip + 1);
  unsigned char *p_data = reinterpret_cast<unsigned char *>(udp + 1);

  ip->ip_v = 4;
  ip->ip_hl = sizeof(click_ip) >> 2;
  ip->ip_len = htons(p->length());
  ip->ip_id = htons(_id.fetch_and_add(1));
  ip->ip_p = IP_PROTO_UDP;
  ip->ip_src = _saddr;
  if (_use_dst_anno)
      ip->ip_dst = p->dst_ip_anno();
  else {
      ip->ip_dst = _daddr;
      p->set_dst_ip_anno(IPAddress(_daddr));
  }
  ip->ip_tos = 0;
  ip->ip_off = 0;
  ip->ip_ttl = 250;

  ip->ip_sum = 0;
#if HAVE_FAST_CHECKSUM && FAST_CHECKSUM_ALIGNED
  if (_aligned)
    ip->ip_sum = ip_fast_csum((unsigned char *)ip, sizeof(click_ip) >> 2);
  else
    ip->ip_sum = click_in_cksum((unsigned char *)ip, sizeof(click_ip));
#elif HAVE_FAST_CHECKSUM
  ip->ip_sum = ip_fast_csum((unsigned char *)ip, sizeof(click_ip) >> 2);
#else
  ip->ip_sum = click_in_cksum((unsigned char *)ip, sizeof(click_ip));
#endif

  p->set_ip_header(ip, sizeof(click_ip));

  EVP_CIPHER_CTX ctx;
  unsigned char key[32] = {0}; // TODO: something fake
  unsigned char iv[16] = {0};
  unsigned char encrypted[4096];
  int outlen = 0, totallen = 0;

  EVP_EncryptInit(&ctx, EVP_aes_128_cbc(), key, iv);
  EVP_EncryptUpdate(&ctx, encrypted, &outlen, p_data, p->length() - sizeof(click_udp) + sizeof(click_ip));
  totallen += outlen;
  EVP_EncryptFinal(&ctx, encrypted + outlen, &outlen);
  totallen += outlen;

  memcpy(p_data, encrypted, p->length() - sizeof(click_udp) + sizeof(click_ip));

  // set up UDP header
  udp->uh_sport = _sport;
  udp->uh_dport = _dport;
  uint16_t len = p->length() - sizeof(click_ip);
  udp->uh_ulen = htons(len);
  udp->uh_sum = 0;
  unsigned csum = click_in_cksum((unsigned char *)udp, len);
  udp->uh_sum = click_in_cksum_pseudohdr(csum, ip, len);

  return p;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(AESForward)
ELEMENT_LIBS(-lssl -lcrypto)
