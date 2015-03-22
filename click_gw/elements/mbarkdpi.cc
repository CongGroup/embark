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
#include <cstring>
#include <click/standard/alignmentinfo.hh>
#include <click/paste.hh>
#include <iostream>
#include <click/router.hh>
#include <click/dpdk.h>
#include <click/dpdk.hh>


/* DPDK SHENANIGANS */
#include <rte_config.h>
#include <rte_common.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_tailq.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>


#include "mbarkdpi.hh"


CLICK_DECLS

extern struct rte_mempool *pktmbuf_pool;
MBArkDPI::MBArkDPI()
{
  //SET ALL THE DIVIDERS
  dividers['"'] = true;
  dividers['.'] = true;
  dividers['`'] = true;
  dividers['~'] = true;
  dividers['\''] = true;
  dividers['/'] = true;
  dividers['<'] = true;
  dividers['>'] = true;
  dividers['?'] = true;
  dividers['='] = true;
  dividers['-'] = true;
  dividers['_'] = true;
  dividers[':'] = true;
  dividers[';'] = true;
  dividers['\\'] = true;
  dividers['$'] = true;
  dividers['#'] = true;
  dividers['*'] = true;
  dividers['+'] = true;
  dividers['('] = true;
  dividers[')'] = true;
  dividers['.'] = true;
  dividers['{'] = true;
  dividers['}'] = true;
  dividers['['] = true;
  dividers[']'] = true;
  dividers['%'] = true;
  dividers['&'] = true;
  dividers['^'] = true;
  dividers['@'] = true;

  //And setup your mempool.
  memset(mystrings, 0, sizeof(char) * 1500 * 32);
  memset(csstrings, 0, sizeof(content_string) * 1500);
  memset(nodes, 0, sizeof(cs_list_node) * 1500);

  freed = 0;
  allocd = 0;

  allocd = 0;
  for(int i = 0; i < 1500; i++){
    cs_list_node* n = nodes + i;
    n->next = freed;
    freed = n;
    n->str = csstrings + i;
    n->str->s = (char*) &(mystrings[i * 32]);
    n->str->strlen = 32;
  }

  char* keyistr = "blueblueblueblue";
  MY_AES_set_encrypt_key((unsigned char*) keyistr, 128, &(key));

}

MBArkDPI::~MBArkDPI()
{
}

  int
MBArkDPI::configure(Vector<String> &conf, ErrorHandler *errh)
{
}


  void
MBArkDPI::push(int, Packet* p)
{
  //std::cout << "Beep!" << std::endl;
  uint32_t len = p->length();
  struct click_ip* iph = (struct click_ip*) p->data();
  
  /*
  if((iph->ip_v != 4) || (ntohs(iph->ip_len) != len)){
    std::cout << "Something is weird with this packet. Did you forget to strip the Ethernet header? Is it fragmented? " << iph->ip_len << " " << len << std::endl;
    p->kill();
    return;
  }
  */

  char* dataptr = ((char*) iph) + sizeof(struct click_ip);
  struct click_tcp* tcph = (click_tcp*) dataptr;
  if(iph->ip_p == IP_PROTO_TCP && (ntohs(tcph->th_sport) == 80 || ntohs(tcph->th_dport) == 80)){
    dataptr += sizeof(struct click_tcp);
  }else{
    output(0).push(p);
    return;
  }

  int bytes = (uint64_t) p->end_data() - (uint64_t) dataptr;

  if (bytes >= MIN_TOKEN_SIZE){
    //IF there are any tokens to send, do it.
    //std::cout << "Let's alloc a packet!" << std::endl;
    struct rte_mbuf *m;
    m = rte_pktmbuf_alloc(pktmbuf_pool);
    unsigned char* seg_buf = (unsigned char*) m->pkt.data;
    int bytesin = 0;
    int max_pkt_len = 1400;
    bytesin = sizeof(struct click_ip) + sizeof(struct click_tcp);
    rte_memcpy(seg_buf, iph, bytesin);

    //std::cout << " I love dpdk " << std::endl;

    //std::cout << "This packet has " << bytes << " bytes of payload!" << std::endl;
    cs_list_node_t* ln = get_user_tokens_from_buffer_easy_3(dataptr, bytes, dividers, &freed);
    //std::cout << "Switches love packets." << std::endl;
    int tot = 0;
    while(ln != 0){
      //These are null terminated because they are all plaintext
      //assert(cur->str->type == PLAINTEXT)i;
      tot++;
      //std::cout << "Let's encrypt!" << std::endl;
      unsigned char out[20];
      MY_AES_encrypt((unsigned char*)ln->str->s, out, &(key));
      rte_memcpy(seg_buf + bytesin, out, 5);
      //MY_AES_encrypt((unsigned char*)ln->str->s, (seg_buf + bytesin), &(key));
      bytesin += 5;
      //std::cout << "bang bang!" <<std::endl;

      if(bytesin >= max_pkt_len){
        //Send token packet and make a new one.
        m->pkt.data_len = bytesin;
        m->pkt.pkt_len = m->pkt.data_len;
        WritablePacket* release = Packet::make(rte_pktmbuf_mtod(m, u_char*), rte_pktmbuf_pkt_len(m), (u_char*) m->buf_addr, m->buf_len, &Packet::dest, true);
        output(1).push(release);

        m = rte_pktmbuf_alloc(pktmbuf_pool);
        seg_buf = (unsigned char*) m->pkt.data;
        bytesin = 0;
        if(iph->ip_p == IP_PROTO_TCP){

          bytesin = sizeof(struct click_ip) + sizeof(struct click_tcp);
        }else{ //Must be UDP

          bytesin = sizeof(struct click_ip) + sizeof(struct click_tcp);
        }
        rte_memcpy(seg_buf, iph, bytesin);
      }
      cs_list_node_t* to_free = ln;
      ln = ln->next;
      to_free->next = freed;
      freed = to_free;
    }
    // std::cout << "-- made " << tot << " tokens!" << std::endl;


    //Send ant tokenpacket data left
    m->pkt.data_len = bytesin;
    m->pkt.pkt_len = m->pkt.data_len;
    WritablePacket* release = Packet::make(rte_pktmbuf_mtod(m, u_char*), rte_pktmbuf_pkt_len(m), (u_char*) m->buf_addr, m->buf_len, &Packet::dest, true);
    output(1).push(release);

  }
  output(0).push(p);
  return;

}

CLICK_ENDDECLS
EXPORT_ELEMENT(MBArkDPI)
ELEMENT_LIBS(-lssl -lcrypto)
