// -*- c-basic-offset: 4 -*-
/*
 * synch_queue_wrapper.cc -- A queue class for record/replay scheduing
 * Justine Sherry
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

#include <pthread.h>
#include <iostream>
#include <click/config.h>
#include <click/error.hh>
#include <click/args.hh>
#include "synch_queue_wrapper.hh"
CLICK_DECLS

SynchQueueWrapper::SynchQueueWrapper()
{
    _xhead = _xtail = 0;
    cur_threadid = 0;
    cur_thread_timesheld = 0;
}

SynchQueueWrapper::~SynchQueueWrapper()
{
}

void *
SynchQueueWrapper::cast(const char *n)
{
    if (strcmp(n, "SynchQueueWrapper") == 0)
	return (SynchQueueWrapper *)this;
    else
	return FullNoteQueue::cast(n);
}

int SynchQueueWrapper::initialize(ErrorHandler * errh){
  //First, check if output port wrapping properly configured
  //

  //TODO TODO: BROKE
/*  Port outwrapper = output(1);
  Port inwrapper = input(1);

  if(! ((noutputs() == 3 && ninputs() == 2) || (noutputs() == 2 && ninputs() == 1))){
  errh->error("Output port [1] must attach to same element as input port [1], or have no wrapped element. Invalid number of port attachments.");
  return -1;
  }
  if((noutputs() == 3 && outwrapper.element() != inwrapper.element())){
    std::cout << outwrapper.element() << " " << inwrapper.element() << std::endl;
    errh->error("Output port [1] must attach to same element as input port [1]; synchqueue wrapped to different elements.");
    return -1;
  }*/

  //Second, grab input stream for replay, if needed.
  //TODO TODO TODO!!!!
    return SimpleQueue::initialize(errh);
  }

int SynchQueueWrapper::configure(Vector<String> &conf, ErrorHandler *errh){
  String filename;
  std::cout << "Reading log file " << conf.size() << std::endl;
  for(int i = 0; i < conf.size(); i++){
    std::cout << "ARGVAL: " << (conf[i].c_str()) << std::endl;
  }
  if(Args(conf, errh).read_p("INLOG", FilenameArg(), filename).execute() < 0)
    return -1;
  std::cout << "My log file is called... " << filename.c_str() << std::endl;

  return FullNoteQueue::configure(conf, errh);
}

int
SynchQueueWrapper::live_reconfigure(Vector<String> &conf, ErrorHandler *errh)
{
    int r = NotifierQueue::live_reconfigure(conf, errh);
    if (r >= 0 && size() < capacity() && _q)
	_full_note.wake();
    _xhead = _head;
    _xtail = _tail;
    return r;
}

void
SynchQueueWrapper::push(int port, Packet *p)
{
    
    //TWO input ports: 0 and 1. 0 is packets coming in from outside
    //world, 1 is packets coming in from serialized element.
    //We spinlock on packets coming in on 0 until serialized element
    //completes. Packets coming in on 1 are from serialized element
    //and thus already have the lock.
    //
    
    if(port == 1){
      //std::cout << "packet in on port 1!" << std::endl;
      push_success(wrap_h, wrap_t, wrap_nt, p);
      return;
    }
    // Code taken from SimpleQueue::push().
    // Reserve a slot by incrementing _xtail
    Storage::index_type t, nt;
    do {
	t = _tail;
	nt = next_i(t);
    } while (_xtail.compare_swap(t, nt) != t);
    // Other pushers spin until _tail := nt (or _xtail := t)

    Storage::index_type h = _head;
    if (nt != h){

      //std::cout << "Push from! " << pthread_self() << std::endl;
      //TODO: Implement batching here so you're not generating packets 1:1
      
      //Store these for when the packet comes back!
      wrap_h = h;
      wrap_t = t;
      wrap_nt = nt;


      uint64_t threadid = pthread_self();
      if(threadid == cur_threadid){
        cur_thread_timesheld++;
      }
      else{
        char* data = new char[10];
        memcpy(data, &cur_threadid, 8);
        memcpy(&(data[8]), &cur_thread_timesheld, 2);
        WritablePacket* newpkt = Packet::make((const void*) data, 10);
        output(0).push(newpkt);
        
        cur_threadid = threadid;
        cur_thread_timesheld = 1;
      }

      output(1).push(p);
      //std::cout << "packet in on port 0!" << std::endl;
      //exitport: output(0).push(newpkt);
    }
    else {
	    _xtail = t;
	    push_failure(p);
    }
}

Packet *
SynchQueueWrapper::pull(int)
{
    // Code taken from SimpleQueue::deq.

    // Reserve a slot by incrementing _xhead
    Storage::index_type h, nh;
    do {
	h = _head;
	nh = next_i(h);
    } while (_xhead.compare_swap(h, nh) != h);
    // Other pullers spin until _head := nh (or _xhead := h)

    Storage::index_type t = _tail;
    if (t != h)
	return pull_success(h, nh);
    else {
	_xhead = h;
	return pull_failure();
    }
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(FullNoteQueue)
EXPORT_ELEMENT(SynchQueueWrapper)
