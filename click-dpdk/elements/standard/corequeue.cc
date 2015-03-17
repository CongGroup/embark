// -*- c-basic-offset: 4 -*-
/*
 * simplequeue.{cc,hh} -- queue element
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
#include "corequeue.hh"
#include <click/args.hh>
#include <click/error.hh>
CLICK_DECLS

CoreQueue::CoreQueue()
    :  _counter(0)
{
}

CoreQueue::~CoreQueue()
{
}
/*
void *
CoreQueue::cast(const char *n)
{
    if (strcmp(n, "Storage") == 0)
	return (Storage *)this;
    else if (strcmp(n, "CoreQueue") == 0
	     || strcmp(n, "Queue") == 0)
	return (Element *)this;
    else
	return 0;
}
*/
int
CoreQueue::configure(Vector<String> &conf, ErrorHandler *errh)
{
    unsigned new_capacity = 1000;
    if (Args(conf, this, errh).read_p("CAPACITY", new_capacity).complete() < 0)
	return -1;
    //_capacity = new_capacity;
    return 0;
}

int
CoreQueue::initialize(ErrorHandler *errh)
{
    //  assert(!_q && _head == 0 && _tail == 0);
    //_q = (Packet **) CLICK_LALLOC(sizeof(Packet *) * (_capacity + 1));
    //if (_q == 0)
    //	return errh->error("out of memory");
    _drops = 0;
    _highwater_length = 0;
    return 0;
}

void
CoreQueue::cleanup(CleanupStage)
{
    /*
    for (Storage::index_type i = _head; i != _tail; i = next_i(i))
	_q[i]->kill();
    CLICK_LFREE(_q, sizeof(Packet *) * (_capacity + 1));
    _q = 0;
    */
    for( ; _counter > 0; _counter--)
	pkt_queue[_counter]->kill();
}

void
CoreQueue::push(int, Packet *p)
{
    simple_enq(p);
   /*
    // If you change this code, also change NotifierQueue::push()
    // and FullNoteQueue::push().
    Storage::index_type h = _head, t = _tail, nt = next_i(t);

    // should this stuff be in CoreQueue::enq?
    if (nt != h) {
	_q[t] = p;
	packet_memory_barrier(_q[t], _tail);
	_tail = nt;

	int s = size(h, nt);
	if (s > _highwater_length)
	    _highwater_length = s;

    } else {
	// if (!(_drops % 100))
	if (_drops == 0 && _capacity > 0)
	    click_chatter("%{element}: overflow", this);
	_drops++;
	checked_output_push(1, p);
    }
    */
}

Packet *
CoreQueue::pull(int)
{
    //   return deq();
    return simple_deq();
}


String
CoreQueue::read_handler(Element *e, void *thunk)
{
    CoreQueue *q = static_cast<CoreQueue *>(e);
    int which = reinterpret_cast<intptr_t>(thunk);
    switch (which) {
      case 0:
	  //return String(q->size());
      case 1:
	return String(q->highwater_length());
      case 2:
	  //return String(q->capacity());
      case 3:
	return String(q->_drops);
      default:
	return "";
    }
}

/*
void
CoreQueue::reset()
{
    while (Packet *p = pull(0))
	checked_output_push(1, p);
}
*/
int
CoreQueue::write_handler(const String &, Element *e, void *thunk, ErrorHandler *errh)
{
    CoreQueue *q = static_cast<CoreQueue *>(e);
    int which = reinterpret_cast<intptr_t>(thunk);
    switch (which) {
      case 0:
	q->_drops = 0;
	//q->_highwater_length = q->size();
	return 0;
      case 1:
	  //q->reset();
	return 0;
      default:
	return errh->error("internal error");
    }
}

void
CoreQueue::add_handlers()
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
//ELEMENT_PROVIDES(Storage)
EXPORT_ELEMENT(CoreQueue)
