#include <click/config.h>
#include <click/glue.hh>
#include <click/pal.hh>
#include <click/packet.hh>
#include <click/master.hh>
#include <click/router.hh>
#include <click/atomic.hh>
#include <iostream>

//Only master/owner thread shoul ever call this function!
void appendPAL(PALSendBuffer* sb, PALEntry* pa, uint8_t thread){
  assert(thread == sb->parent);
//  std::cout << "sb->head:" << sb->head << " sb->tail:" << sb->tail << std::endl;
  assert(sb->head >= sb->tail);
  assert((sb->head - sb->tail) <= (uint32_t) MAX_PAL_ENTRIES); //Need to have enough room.

  //set the sequence number
  pa->pal_seq_no = sb->pal_seq_no;
  sb->pal_seq_no++;

  //Do the wraparound!
  int16_t t = sb->tail;
  if(t >= MAX_PAL_ENTRIES){
    assert(t < MAX_PAL_ENTRIES * 2);
    sb->head = t % MAX_PAL_ENTRIES;
    sb->tail = t % MAX_PAL_ENTRIES;
  }

  //Actually do the copy
  uint32_t h = sb->head;
  memcpy(&(sb->buffer[h % MAX_PAL_ENTRIES]), pa, sizeof(PALEntry));
  sb->head = (h + 1);
}

uint8_t copyPALsToBuffer(void* buffer, PALSendBuffer* sb, uint8_t max_PALs){
  if(max_PALs == 0) return 0;
  uint32_t t,h;
  do{ //in case wraparound operation is being performed RIGHT NOW.
    t = sb->tail;
    h = sb->head;
  } while(t > h);
  if(t == h) return 0;

  PALEntry* dst = (PALEntry*) buffer;
  uint8_t i;
  //Have to do this because of wrap!
  for(i = 0; i < max_PALs && (t + i) < h; i++){
    memcpy(((void*) &(dst[i])),  (void*) &(sb->buffer[(i + t) % MAX_PAL_ENTRIES]), sizeof(PALEntry));
  }

  sb->tail = (t + i);
  return i;
}


PALEntry* stripPacket(PALHeader* headercpy, uint8_t* PALcount, Packet* p){
  PALHeader* header = (PALHeader*) p->data();
  memcpy(headercpy, header, sizeof(PALHeader));
  //p->seqno = ntohl(header->pkt_id);
  //VECTOR SEQNOS??? TODO TODO  
  p->pull(sizeof(PALHeader));
  return NULL;
}

