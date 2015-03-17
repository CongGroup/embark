// -*- c-basic-offset: 4 -*-
/*
 * MultiThreadMultiQueue.{cc,hh} -- queue element
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
#include <iostream>
#include "multithreadmultiqueue.hh"
#include <click/router.hh>
#include <click/master.hh>
#include <click/args.hh>
#include <click/error.hh>
CLICK_DECLS

MultiThreadMultiQueue::MultiThreadMultiQueue()
    : _q(0)
{
}

MultiThreadMultiQueue::~MultiThreadMultiQueue()
{
}

void *
MultiThreadMultiQueue::cast(const char *n)
{
    if (strcmp(n, "Storage") == 0)
	return (Storage *)this;
    else if (strcmp(n, "MultiThreadMultiQueue") == 0
	     || strcmp(n, "Queue") == 0)
	return (Element *)this;
    else
	return 0;
}

int
MultiThreadMultiQueue::configure(Vector<String> &conf, ErrorHandler *errh)
{
    unsigned new_capacity = 1000;
    _nthreads = master()->nthreads();
    String fn;

    if (Args(conf,  errh).read_p("CAPACITY", new_capacity).consume() < 0)
	return -1;
    _capacity = new_capacity;
    return 0;
}

int
MultiThreadMultiQueue::initialize(ErrorHandler *errh)
{
    std::cout << "Initializing multithreadmultiqueue!!!" << std::endl;
    assert(!_q);
    _heads = (index_type*) CLICK_LALLOC(sizeof(index_type) * _nthreads);
    for(int i = 0; i < _nthreads; i++) _heads[i] = 0;
    _tails = (index_type*) CLICK_LALLOC(sizeof(index_type) * _nthreads);
    for(int i = 0; i < _nthreads; i++) _tails[i] = 0;

//    _q = (Packet ***) CLICK_LALLOC(sizeof(Packet *) * (_capacity + 1));

    _q  = (Packet ***) CLICK_LALLOC (sizeof(Packet**) * _nthreads);
    for(int i = 0; i < _nthreads; i++){
      _q[i] = (Packet **) CLICK_LALLOC(sizeof(Packet*) * (_capacity + 1));
      for(int j = 0; j < _capacity + 1; j++){
        _q[i][j] = 0;
      }
      if(_q[i] == 0)
        return errh->error("out of memory");
    }
    
    //_packetdata = new Packet*[_nthreads * (_capacity +1)];

     if (_q == 0)
	      return errh->error("out of memory");
    


   _drops = 0;
    _highwater_length = 0;
    std::cout << "multiqueues allocated" << std::endl;
    return 0;
    
}

int
MultiThreadMultiQueue::live_reconfigure(Vector<String> &conf, ErrorHandler *errh)
{
  /* This should never be called JUSTINE
    // change the maximum queue length at runtime
    Storage::index_type old_capacity = _capacity;
    // NB: do not call children!
    if (MultiThreadMultiQueue::configure(conf, errh) < 0)
	return -1;
    if (_capacity == old_capacity || !_q)
	return 0;
    Storage::index_type new_capacity = _capacity;
    _capacity = old_capacity;

    Packet **new_q = (Packet **) CLICK_LALLOC(sizeof(Packet *) * (new_capacity + 1));
    if (new_q == 0)
	return errh->error("out of memory");

    Storage::index_type i, j;
    for (i = _head, j = 0; i != _tail && j != new_capacity; i = next_i(i))
	new_q[j++] = _q[i];
    for (; i != _tail; i = next_i(i))
	_q[i]->kill();

    CLICK_LFREE(_q, sizeof(Packet *) * (_capacity + 1));
    _q = new_q;
    _head = 0;
    _tail = j;
    _capacity = new_capacity;*/
    return 0;
}

void
MultiThreadMultiQueue::take_state(Element *e, ErrorHandler *errh)
{
  /*
    MultiThreadMultiQueue *q = (MultiThreadMultiQueue *)e->cast("MultiThreadMultiQueue");
    if (!q)
	return;

    if (_tail != _head || _head != 0) {
	errh->error("already have packets enqueued, can%,t take state");
	return;
    }

    _head = 0;
    Storage::index_type i = 0, j = q->_head;
    while (i < _capacity && j != q->_tail) {
	_q[i] = q->_q[j];
	i++;
	j = q->next_i(j);
    }
    _tail = i;
    _highwater_length = size();

    if (j != q->_tail)
	errh->warning("some packets lost (old length %d, new capacity %d)",
		      q->size(), _capacity);
    while (j != q->_tail) {
	q->_q[j]->kill();8
	j = q->next_i(j);
    }
    q->set_head(0);
    q->set_tail(0);*/
}

void
MultiThreadMultiQueue::cleanup(CleanupStage)
{
  for(uint8_t j = 0; j < _nthreads; j++){
    for (Storage::index_type i = _heads[j]; i != _tails[j]; i = next_i(i))
	    _q[j][i]->kill();
  }
    CLICK_LFREE(_q, sizeof(Packet *) * (_capacity + 1));
    _q = 0;
}


uint8_t
MultiThreadMultiQueue::get_cur_threadid(){
 return 0; 
}



void
MultiThreadMultiQueue::push(int, Packet *p)
{
  
    uint8_t tid = p->_parent_thread;
    Storage::index_type h = _heads[tid], t = _tails[tid], nt = next_i(t);

    //std::cout << this << "tid " << ((uint32_t) tid) << "h " << h << " t " << t << " nt " << nt << std::endl;  
    // should this stuff be in MultiThreadMultiQueue::enq?
    if (nt != h) {
     // std::cout << "packet goes to " <<  _q[tid] << " " << t <<std::endl;
      _q[tid][t] = p;
      //packet_memory_barrier(_q[t], _tail);
      _tails[tid] = nt;

      int s = size(h, nt);
      if (s > _highwater_length)
          _highwater_length = s;

    } else {
	// if (!(_drops % 100))
	
	    //std::cout << "OVERFLOW" << std::endl;
      p->kill();
    if (_drops == 0 && _capacity > 0)
	    _drops++;
    }
}

Packet *
MultiThreadMultiQueue::pull(int tid)
{
  //std::cout << this << " PULLING A PACKET FROM " << tid << std::endl;
  Packet* ret = deq(tid);
  //std::cout << "PULLED " << ret << std::endl;
  return ret;
}


String
MultiThreadMultiQueue::read_handler(Element *e, void *thunk)
{
    /*MultiThreadMultiQueue *q = static_cast<MultiThreadMultiQueue *>(e);
    int which = reinterpret_cast<intptr_t>(thunk);
    switch (which) {
      case 0:
	return String(q->size());
      case 1:
	return String(q->highwater_length());
      case 2:
	return String(q->capacity());
      case 3:
	return String(q->_drops);
      default:*/
	return "";
    //}
}

void
MultiThreadMultiQueue::reset()
{
    while (Packet *p = pull(0))
	checked_output_push(1, p);
}

int
MultiThreadMultiQueue::write_handler(const String &, Element *e, void *thunk, ErrorHandler *errh)
{
    /*MultiThreadMultiQueue *q = static_cast<MultiThreadMultiQueue *>(e);
    int which = reinterpret_cast<intptr_t>(thunk);
    switch (which) {
      case 0:
	q->_drops = 0;
	q->_highwater_length = q->size();
	return 0;
      case 1:
	q->reset();
	return 0;
      default:
	return errh->error("internal error");
    }*/
  return 0;
}

void
MultiThreadMultiQueue::add_handlers()
{
    add_read_handler("length", read_handler, 0);
    add_read_handler("highwater_length", read_handler, 1);
    add_read_handler("capacity", read_handler, 2, Handler::CALM);
    add_read_handler("drops", read_handler, 3);
    add_write_handler("capacity", reconfigure_keyword_handler, "0 CAPACITY");
    add_write_handler("reset_counts", write_handler, 0, Handler::BUTTON | Handler::NONEXCLUSIVE);
    add_write_handler("reset", write_handler, 1, Handler::BUTTON);
}

CLICK_ENDDECLS
ELEMENT_PROVIDES(Storage)
EXPORT_ELEMENT(MultiThreadMultiQueue)
