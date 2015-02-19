/*
   }
 * OrderedReleaseReceiver.{cc,hh} -- do-nothing element
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include <click/args.hh>
#include <click/pal.hh>
#include <clicknet/ether.h>
#include <iostream>
#include <clicknet/ip.h>
#include "orderedrelease.hh"
#include <click/router.hh>

CLICK_DECLS


uint32_t* OrderedReleaseReceiver::neighborseqs = new uint32_t[DP_THREADS]; //it's hard-code oclock JUSTINE

OrderedReleaseReceiver::OrderedReleaseReceiver()
{
  memset(neighborseqs, 0, sizeof(uint32_t) * DP_THREADS);
  lastseqno = 0;
  palpackets = new Packet*[MAX_PAL_ENTRIES];
  memset(palpackets, 0, sizeof(Packet*) * MAX_PAL_ENTRIES);
  datapackets = new Packet*[MAX_PAL_ENTRIES];
  memset(datapackets, 0, sizeof(Packet*) * MAX_PAL_ENTRIES);
  my_idx = 255;

  dpcount = 0;
  dpidx = 0;
}



void OrderedReleaseReceiver::sendNACK(uint32_t wantseq, uint32_t haveseq, click_ether* toreverse){
}


void
OrderedReleaseReceiver::removeLostNode(LostPacketNode* nl){
}

  void
OrderedReleaseReceiver::push(int, Packet *p)
{
  //std::cout << "PACKET!" << std::endl;
  DataPacketHeader* dp = (DataPacketHeader*) p->data();
  if(dp->is_data_packet == 1){
    if(my_idx == 255) my_idx = dp->thread_id;

    //std::cout << "Received data packet on idx " << (uint32_t) my_idx << std::endl;
    bool canrelease = true;
    for(int i = 0; i < DP_THREADS && canrelease; i++){
      if(dp->pal_seqnos[i] > neighborseqs[i]){
        datapackets[dpidx] = p;
        dpcount++;
        dpidx++;
        dpidx = dpidx % MAX_PAL_ENTRIES;
        //std::cout << dpidx << " Can't release! Wanted " << dp->pal_seqnos[i] << " at " << i << " but got" <<  neighborseqs[i] << std::endl;
        canrelease = false;
      }
    }
    if(canrelease){
      //std::cout << "Release... push!!!" << std::endl;
      p->pull(sizeof(DataPacketHeader));
      output(0).push(p);
    }
    //std::cout << "RETURNING" << std::endl;
    return;
  }else if(dp->is_data_packet == 0){
    if(my_idx == 255) my_idx = (uint8_t) *(p->data() + 1);
    //std::cout << "Received PAL on idx " << (uint32_t) my_idx << " length: " << p->length() << std::endl;
    p->pull(2); //No actual data header here, lol
    PALEntry* ptr = (PALEntry*) p->data();
    while(ptr < (PALEntry*) p->end_data()){
      lastseqno = ptr->pal_seq_no;
      //std::cout << (uint32_t) my_idx << " RECEIVED PAL WITH " << ptr->pal_seq_no << std::endl;
      if(OrderedReleaseReceiver::neighborseqs[my_idx] < lastseqno){
        OrderedReleaseReceiver::neighborseqs[my_idx] = lastseqno;
      }
      /*if(ptr->pal_seq_no == lastseqno + 1){
        std::cout << "Received seqno for " << lastseqno << std::endl;
        lastseqno++;
      }
      else{
        std::cout << "LOSS!! expected: " << lastseqno << " RECEIVED: " << ptr->pal_seq_no << std::endl;
        assert(false);
      }*/ //FUCK IT
      ptr = &ptr[1];
    }
    //std::cout << OrderedReleaseReceiver::neighborseqs << " updating table... " << (uint32_t) my_idx <<" " << lastseqno << std::endl;

    //JUSTINE UPDATE PACKET RELEASE
    uint32_t checkidx = (MAX_PAL_ENTRIES + dpidx - dpcount) % MAX_PAL_ENTRIES;
    //std::cout << dpidx << " Can I release?? " << checkidx << std::endl; 
    bool canrelease = true;
    while(datapackets[checkidx] != 0 && canrelease){
      //std::cout << "checking what I can release " << checkidx << std::endl;
      Packet* myp = datapackets[checkidx];
      DataPacketHeader* dp = (DataPacketHeader*) myp->data();
      //std::cout << "have dp.." << std::endl; 
      for(int i = 0; i < DP_THREADS; i++){
        if(dp->pal_seqnos[i] > neighborseqs[i]){
          //std::cout << "Can't release! " << dp->pal_seqnos[i] << " " << neighborseqs[i] << std::endl;  
          canrelease = false;
          break;
        }
      }
      if(canrelease){
        //std::cout << "RELEASING PACKET (LATE!)!" << std::endl;
        datapackets[checkidx] = 0;
        dpcount--;
        output(0).push(myp);
        checkidx = (checkidx + MAX_PAL_ENTRIES - 1) % MAX_PAL_ENTRIES;
      }
    }
    //std::cout << "CLEANUP" << std::endl;
    //STORE/CLEANUP PALS
    p->kill();
  }
}

CLICK_ENDDECLS
EXPORT_ELEMENT(OrderedReleaseReceiver)
ELEMENT_MT_SAFE(OrderedReleaseReceiver)
