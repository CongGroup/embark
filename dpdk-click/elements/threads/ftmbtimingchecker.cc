// -*- c-basic-offset: 4 -*-
/*
 * FTMBTimingChecker.{cc,hh} -- queue element
 * Justine Sherry borrowing heavily from Eddie Kohler
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
#include <time.h>
#include <stdlib.h>
#include <click/standard/scheduleinfo.hh>
#include "ftmbtimingchecker.hh"
#include <click/router.hh>
#include <click/master.hh>
#include <click/args.hh>
#include <click/error.hh>
CLICK_DECLS

  FTMBTimingChecker::FTMBTimingChecker()
: _q(0), _pause_compare(0), _mytask(this), _timer(&_mytask)
{
}

FTMBTimingChecker::~FTMBTimingChecker()
{
}

  void *
FTMBTimingChecker::cast(const char *n)
{
  if (strcmp(n, "Storage") == 0)
    return (Storage *)this;
  else if (strcmp(n, "FTMBTimingChecker") == 0
      || strcmp(n, "Queue") == 0)
    return (Element *)this;
  else
    return 0;
}

  int
FTMBTimingChecker::configure(Vector<String> &conf, ErrorHandler *errh)
{
  unsigned new_capacity = 1000;
  _nthreads = master()->nthreads();
  String fn;

  int s = rand() % 10;

  if (Args(conf,  errh).read_p("CAPACITY", new_capacity).consume() < 0)
    return -1;
  _capacity = new_capacity;
  return 0;
}

bool
FTMBTimingChecker::run_task(Task *){
  //TODO: when this is scheduled, check how long since random last timer.
  //If >, go ahead and (a) LOCK queues, (b) print out stats, (c) UNLOCK queues, (d) reset timer.

  if(_pause_compare) return false;

  _pause_compare = true;

  for(int i = 0; i < _nthreads; i++){
    Packet* p =  _q[i][_tails[i]];
    if(p)
      std::cout << p->dst_ip_anno().unparse().c_str() << std::endl;
  }

  int s = rand() % 100;

   if (_timer.expiry())
	_timer.reschedule_after_msec(s);
    else
	_timer.schedule_after_msec(s);
 
  _pause_compare = false;
  return false;


}

  int
FTMBTimingChecker::initialize(ErrorHandler *errh)
{
  std::cout << "Initializing FTMBTimingChecker!!!" << std::endl;
  assert(!_q);
  _heads = new index_type[_nthreads];
  for(int i = 0; i < _nthreads; i++) _heads[i] = 0;
  _tails = new index_type[_nthreads];
  for(int i = 0; i < _nthreads; i++) _tails[i] = 0;

  _q = (Packet ***) CLICK_LALLOC(sizeof(Packet *) * (_capacity + 1));

  _q  = new Packet**[_nthreads];
  for(int i = 0; i < _nthreads; i++){
    _q[i] = new Packet*[_capacity + 1];
    for(int j = 0; j < _capacity + 1; j++){
      _q[i][j] = 0;
    }
    if(_q[i] == 0)
      return errh->error("out of memory");
  }

  //_packetdata = new Packet*[_nthreads * (_capacity +1)];
  srand(time(NULL));
  _timer.initialize(this);


  if (_q == 0)
    return errh->error("out of memory");

  ScheduleInfo::initialize_task(this, &_mytask, true, errh);

  _drops = 0;
  _highwater_length = 0;
  std::cout << "multiqueues allocated" << std::endl;
  return 0;

}

  int
FTMBTimingChecker::live_reconfigure(Vector<String> &conf, ErrorHandler *errh)
{
  /* This should never be called JUSTINE
  // change the maximum queue length at runtime
  Storage::index_type old_capacity = _capacity;
  // NB: do not call children!
  if (FTMBTimingChecker::configure(conf, errh) < 0)
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
FTMBTimingChecker::take_state(Element *e, ErrorHandler *errh)
{
  /*
     FTMBTimingChecker *q = (FTMBTimingChecker *)e->cast("FTMBTimingChecker");
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
FTMBTimingChecker::cleanup(CleanupStage)
{
  for(uint8_t j = 0; j < _nthreads; j++){
    for (Storage::index_type i = _heads[j]; i != _tails[j]; i = next_i(i))
      _q[j][i]->kill();
  }
  CLICK_LFREE(_q, sizeof(Packet *) * (_capacity + 1));
  _q = 0;
}


uint8_t
FTMBTimingChecker::get_cur_threadid(){
  return 0; 
}



  void
FTMBTimingChecker::push(int, Packet *p)
{

  if(_pause_compare){
    p->kill();
    return;
  }


  uint8_t tid = p->_parent_thread;
  Storage::index_type h = _heads[tid], t = _tails[tid], nt = next_i(t);

  // std::cout << "Push! " << h << std::endl;
  if(nt == h){
    Packet *pold = _q[tid][h];
    if(pold) pold->kill();
    _heads[tid] = next_i(h);
    h = _heads[tid];
  }

  //assert(false);
  //assert(nt != h);

  //std::cout << "packet goes to " <<  _q[tid] << " " << t <<std::endl;
  _q[tid][t] = p;
  //packet_memory_barrier(_q[t], _tail);
  _tails[tid] = nt;
}

//  Packet *
//FTMBTimingChecker::pull(int tid)
//{
//std::cout << "PULLING A PACKET FROM " << tid << std::endl;
//  Packet* ret = deq(tid);
//  //std::cout << "PULLED " << ret << std::endl;
//  return ret;
//}


  String
FTMBTimingChecker::read_handler(Element *e, void *thunk)
{
  /*FTMBTimingChecker *q = static_cast<FTMBTimingChecker *>(e);
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
FTMBTimingChecker::reset()
{
  while (Packet *p = pull(0))
    checked_output_push(1, p);
}

  int
FTMBTimingChecker::write_handler(const String &, Element *e, void *thunk, ErrorHandler *errh)
{
  /*FTMBTimingChecker *q = static_cast<FTMBTimingChecker *>(e);
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
FTMBTimingChecker::add_handlers()
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
EXPORT_ELEMENT(FTMBTimingChecker)
