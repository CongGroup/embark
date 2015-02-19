#ifndef CLICK_FTMB_HH
#define CLICK_FTMB_HH
#include <click/element.hh>
#include <click/atomic.h>
#include <click/mcs_lock.h>
#include <click/pal.hh>
#include <unistd.h>
CLICK_DECLS

//2x the round-trip BDP + some headroom
#define SENDBUFFERSIZE 100000
#define MAX_BUFFER_OCCUPANCE 1000

typedef struct LostPacketNode{
  uint32_t seqno;
  Packet* p;
  LostPacketNode* next;
  LostPacketNode* prev;
} LostPacketNode;

//In the rewrite: should be push - push
//OK if packet buffer overflows (indeed, that's good -- it's how congestion control works)
//NOT okay if PAL buffer overflows. When FromDevice is scheduled, if there's not enough
//PAL buffer space for more packets, just return so that ToDevice can be scheduled.
//
//On push -> insert packet in to ring buffer for release
//On pull -> //FromDevice should have a higher burst size than todevice to account for PAL ppackets.
//  -- If there are PALs in the PAL Queue, release those.
//  -- Else if there are packets which have already been assigned a thread-vector-clock, release one of those
//  -- Else, create a new thread vector clock (looking at PAL seqs for all threads) and append to all packets in the queue.
//
//PAL Queue now has a "PAL Seq" that gets assigned to each PAL upon entry.
//
//Enfroce reliablity for PALs, but not for packets.

class OrderedReleaseReceiver : public Element { public:

  OrderedReleaseReceiver();

  const char *class_name() const	{ return "OrderedReleaseReceiver"; }
  const char *port_count() const	{ return "1/1"; }

  void push(int, Packet *);

  static uint32_t* neighborseqs;
  private:
  LostPacketNode* lostlistelt(uint32_t seq);
  void removeLostNode(LostPacketNode* nl);
  void sendNACK(uint32_t ackseq, uint32_t rseq, click_ether* toreverse);
 
  uint32_t lastseqno;
  uint8_t my_idx;

  Packet** palpackets;
  uint32_t ppidx;
  Packet** datapackets;
  uint32_t dpidx;
  uint32_t dpcount;
};

CLICK_ENDDECLS
#endif
