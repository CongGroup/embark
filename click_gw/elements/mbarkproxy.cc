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
  //click_chatter("trying to parse an HTTP packet. len: %d", len);

  //char buf[1024] = {0};
  //strncpy(buf, data, std::min(len, 64));
  //click_chatter("%s", buf);

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
          url_end = s - 2; 
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
  url.append(p, q-1);

  //click_chatter("%s", url.c_str());
  return true;
}

void
MBArkProxy::push(int, Packet *p)
{
  const click_ip6 *ip = (click_ip6 *)p->data();
  const ext_hdr *option = (ext_hdr *)(ip + 1);
  const click_tcp *tcp = (click_tcp *)(option + 1);

  assert(option->hdr_ext_len == 6);

  if (option->next_hdr == 6 && ntohs(tcp->th_dport) == 80) 
  {
    int ip_plen = ntohs(ip->ip6_plen);
    int d_offset = tcp->th_off * 4;

    const char *data = (const char *) tcp;
    data += d_offset;

    //click_chatter("ip_plen: %d, ext_hdr: %d, d_offset: %d", ip_plen, sizeof(ext_hdr), d_offset);

    int data_len = ip_plen - d_offset - sizeof(ext_hdr);

    if (data_len > 0) {
      String url;
      parse_http_req(data, data_len, url);
    }
  } else {
    //click_chatter("nxt_hdr: %x", option->next_hdr);
    //if (option->next_hdr == 6) click_chatter("port: %d", ntohs(tcp->th_dport));
  }

  output(0).push(p);

}

CLICK_ENDDECLS
EXPORT_ELEMENT(MBArkProxy)
ELEMENT_LIBS(-lssl -lcrypto)
