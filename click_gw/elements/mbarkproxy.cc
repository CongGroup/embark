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

#include "mbarkproxy.hh"
#include "mbarkfw.hh"


CLICK_DECLS

MBArkProxy::MBArkProxy()
{
}


MBArkProxy::~MBArkProxy()
{
}

int
MBArkProxy::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if (Args(conf, this, errh)
  //.read_mp("FILENAME", FilenameArg(), _ff.filename())
  .complete() < 0)
    return -1;
  return 0;
}

bool
MBArkProxy::parse_http_req(const char *data, int len, String& url)
{
  if (strncmp(data, "GET ", 4) != 0)
    return false;

  const char *p = data + 4, *q = p, *end = data + len;
  
  while (q < end && *q != ' ') {
    ++q;
  }
  
  if (q == end)
    return false;

  // url is [p, q-1]
  assert(p <= q-1);

  if (*p == '/') 
  {
    const char *s = data, *url_start = nullptr, *url_end = nullptr;
    int state = 0; // find Host
    for (; s < end && url_end == nullptr; ++s) {
      switch (state) {
      case 0:
        if (strncmp(s, "Host: ", 6) == 0) {
          state = 1; // find next \r, then \n, then finish
          s += 5;
          url_start = s + 1;
        } else {
          state = 2; // find next \r, then \n, then Host
          s += 5;
        }
        break;
      case 1:
        if (*s == '\r') {
          state = 3; // find next \n, then finish
        }
        break;
      case 2:
        if (*s == '\r') {
          state = 4; // find next \n, then Host
        }
      case 3:
        if (*s == '\n') {
          url_end = s - 1; 
        }
        break;
      case 4:
        if (*s == '\n') {
          state = 0;
        }
        break;
      default:
        break;
      }
    }
    if (url_end == nullptr) 
      return false;
    url.append(url_start, url_end);
  }
  url.append(p, q);

  //click_chatter("%s", url.c_str());
  return true;
}

void
MBArkProxy::push(int, Packet *p)
{
  const click_ip6 *ip6 = (const click_ip6 *) p->data();
  const click_tcp *tcp = nullptr;
  
  int data_len = 0;
  const char *p_data = ((const char *) tcp) + tcp->th_off * 4;

  if (ip6->ip6_v == 6) 
  {
    if (ip6->ip6_nxt == 60) 
    {
      const ext_hdr *option = (ext_hdr *)(ip6 + 1);
      tcp = (click_tcp *)(option + 1);
      data_len = ntohs(ip6->ip6_plen) - tcp->th_off * 4 - sizeof(ext_hdr);

      assert(option->hdr_ext_len == 6);
    } 
    else if (ip6->ip6_nxt == 6) 
    {
      tcp = (click_tcp *)(ip6 + 1);
      data_len = ntohs(ip6->ip6_plen) - tcp->th_off * 4;
    } 
    else 
    {
      click_chatter("unknown ipv6 packet.");
      //tcp = (click_tcp *)(ip6 + 1);
    }
  }
  else
  {
    click_chatter("non-ipv6 packet.");
  } 
  /*
  else if (*data & 0xf == 4) 
  {
    const click_ip *ip = (click_ip *) data;
    if (ip->ip_p == 6) 
    {
      tcp = (click_tcp *) (data + ip->ip_hl * 4);
      data_len = ntohs(ip->ip_len) - ip->ip_hl * 4 - tcp->th_off * 4;
    }
  } 
  */

  if (tcp != nullptr && ntohs(tcp->th_dport) == 80) {
    const char *p_data = ((const char *) tcp) + tcp->th_off * 4;
    //click_chatter("ip_plen: %d, ext_hdr: %d, d_offset: %d", ip_plen, sizeof(ext_hdr), d_offset);

    if (data_len > 0) {
      String url;

      if (parse_http_req(p_data, data_len, url)) {
        int url_len = url.length();
        click_chatter("url: %s", url.c_str());

        WritablePacket *q = Packet::make(sizeof(click_ip6) + sizeof(click_udp) + url_len + 1);
        memset(q->data(), '\0', q->length());

        click_ip6 *ip6_new = (click_ip6 *)q->data();
        click_udp *udp_new = (click_udp *)(ip6_new + 1);
        char *payload = (char *)(udp_new + 1);

        memcpy(ip6_new, ip6, sizeof(click_ip6));

        ip6_new->ip6_plen = htons(sizeof(click_udp) + url_len + 1);
        ip6_new->ip6_nxt = 17;

        udp_new->uh_sum = 0;
        udp_new->uh_ulen = htons(sizeof(click_udp) + url_len + 1);
        
        strcpy(payload, url.c_str());

        udp_new->uh_sum = htons(in6_fast_cksum(&ip6_new->ip6_src, &ip6_new->ip6_dst, udp_new->uh_ulen, 
          ip6_new->ip6_nxt, udp_new->uh_sum, (unsigned char *)(udp_new), udp_new->uh_ulen)); 

        output(1).push(q);
      }
    }
  }

  output(0).push(p);

}

CLICK_ENDDECLS
EXPORT_ELEMENT(MBArkProxy)
ELEMENT_LIBS(-lssl -lcrypto)
