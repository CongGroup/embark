// -*- related-file-name: "../../lib/pal.cc" -*-

#ifndef PACKET_ACCESS
#define PACKET_ACCESS

//How many PALs to keep around for retransmission
#define MAX_PAL_ENTRIES 10000

class RouterThread;
class Packet;

/**
 * The Packet Access Log. 64 bytes.
 */
typedef struct __attribute__((__packed__)) PALEntry{
      uint64_t pal_seq_no; 
      uint32_t pkt_id;
      uint8_t lock_id;
      uint32_t lock_seq;
} PALEntry;

/**
 *
 * A ring buffer structure and some assorted methods for storing PALs at the thread level.
*/ 
typedef struct PALSendBuffer{
  uint64_t pal_seq_no;
  uint32_t head;
  uint32_t tail;
  uint8_t parent;
  RouterThread* tparent;
  PALEntry buffer[MAX_PAL_ENTRIES];
} PALSendBuffer;


/**
 * What goes on the header!
 */
//I am hardcoding this right now because graphs.
#define DP_THREADS 4
typedef struct __attribute__((__packed__)) DataPacketHeader{
  uint8_t is_data_packet;
  uint32_t pkt_id;
  uint8_t thread_id; 
  uint64_t pal_seqnos[DP_THREADS];
} PALHeader;

//Only master/owner thread shoul ever call this function!
void appendPAL(PALSendBuffer* sb, PALEntry* pa, uint8_t thread);
uint8_t copyPALsToBuffer(void* buffer, PALSendBuffer* sb, uint8_t max_PALs);

PALEntry* stripPacket(PALHeader* headercpy, uint8_t* PALcount, Packet* p);

#endif
