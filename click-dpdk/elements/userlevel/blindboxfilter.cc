#include <click/config.h>
#include <click/args.hh>
#include <click/error.hh>
#include <click/router.hh>
#include <click/standard/scheduleinfo.hh>
#include <click/glue.hh>
#include <click/straccum.hh>
#include <click/handlercall.hh>
#include <click/signaturetable.hh>
#include <clicknet/ip.h>
#include <clicknet/ether.h>
#include <clicknet/tcp.h>
#include <click/glue.hh>
#include <arpa/inet.h>
#include "blindboxfilter.hh"
CLICK_DECLS

BlindBoxFilter::BlindBoxFilter() 
{
}

BlindBoxFilter::~BlindBoxFilter()
{
}

  void *
BlindBoxFilter::cast(const char *n)
{
  if (strcmp(n, class_name()) == 0)
    return static_cast<Element *>(this);
  else
    return 0;
}

  int
BlindBoxFilter::configure(Vector<String> &conf, ErrorHandler *errh)
{
}

  int
BlindBoxFilter::initialize(ErrorHandler *errh)
{
  gt = new GlobalSignatureTable();
  st = new ThreadLocalSignatureTable();
  gt->appendThreadLocalSignatureTable(st);
  //Todo:setup
}

  void
BlindBoxFilter::cleanup(CleanupStage)
{
}

  void
BlindBoxFilter::push(int port, Packet* p)
{
  const unsigned char* dp = p->data();
  const click_ether* eth = reinterpret_cast<const click_ether*>(p->data());
  if(ntohs(eth->ether_type) == ETHERTYPE_IP){
    const click_ip* ip = (const click_ip*) (p->data() + sizeof(click_ether));

    //The "primary" channel.
    if(ip->ip_p == IP_PROTO_TCP){
      const click_tcp* tcp = (const click_tcp*)(((char*) ip) + sizeof(click_ip));

      /**LOGGING**/
      char s_src[16];
      char s_dst[16];
      strcpy(s_src, inet_ntoa(ip->ip_src));
      strcpy(s_dst, inet_ntoa(ip->ip_dst));
      std::cout << "TCP PACKET:" << "V: " << (uint32_t) ip->ip_v << " ID:" << ntohs(ip->ip_id) <<   " HL:" << (uint32_t) ip->ip_hl << " SADDR: " << s_src <<"/" << ntohs(tcp->th_sport) << " DADDR: " <<  s_dst << "/" << ntohs(tcp->th_dport) << " SEQNO:" << ntohl(tcp->th_seq) << std::endl; 
      
      /** The connection ID **/
      ConnectionID id;
      if(ip->ip_src.s_addr < ip->ip_dst.s_addr){
        id.ip1 = ip->ip_src.s_addr;
        id.ip2 = ip->ip_dst.s_addr;
        id.sp1 = tcp->th_sport;
        id.sp2 = tcp->th_dport;
      }else{
        id.ip1 = ip->ip_dst.s_addr;
        id.ip2 = ip->ip_src.s_addr;
        id.sp1 = tcp->th_dport;
        id.sp2 = tcp->th_sport;
      }
      ConnRecord* r = st->getConnRecord(&id);
      ConnStatus status = r ? r->status : NO_RECORD;
      
      /** Is it a SYN? **/
      if((tcp->th_flags & TH_SYN) == TH_SYN){
        if(status == NO_RECORD){
          std::cout << "Hey, this is a new SYN! Inserting " << s_src << " " << s_dst << std::endl;  
          if(!st->addToPendingConnections(&id, ip->ip_src.s_addr, ntohl(tcp->th_seq))){
            std::cout << "Pending connecitons queue is all full! :( Dropping packet." << std::endl;
            p->kill();
            return;
          }
        }else if(status == PENDING){
          
        }
      } 
      else if(status != NO_RECORD){
        std::cout << "Yo dog, packet from: " << inet_ntoa(ip->ip_src) << " proto: " << (uint32_t) ip->ip_p << std::endl;
        if(status == ALLOW){
          std::cout << "And it is allowed!" << std::endl;
        }else if (status == PENDING){
          std::cout << "And it is pending!" << std::endl;
        }else if (status == DENY){
          std::cout << "And it is blocked!" << std::endl;
        }

      }else{
        std::cout << "UNKNOWN!!!" << std::endl;
      }
    }

    //(1) Is connection setup? 
    //(2) Has validation proceeded past this byte?
    //If yes, forward. If no, drop.
    else if(ip->ip_p == IP_PROTO_UDP){
      //      std::cout << "UDP packet..." << std::endl;
    }
    else{
      std::cout << "What is this packet doing in our testbed?!" << std::endl;
    }
  }else{
    std::cout << "Packet but it's like ARP or something: "<< std::hex << htons(eth->ether_type) << std::dec << std::endl;
  }
  output(0).push(p);
}

  CLICK_ENDDECLS
EXPORT_ELEMENT(BlindBoxFilter)
