/*
 * nullelement.{cc,hh} -- do-nothing element
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
#include "nullelement.hh"
#include <iostream>
#include <click/task.hh>
#include <click/routerthread.hh>
#include <click/router.hh>
#include <click/master.hh>
#include <click/glue.hh>
#include <click/args.hh>
#include <click/task.hh>
#include <click/error.hh>


CLICK_DECLS

NullElement::NullElement()
{
}

NullElement::~NullElement()
{
}

  Packet *
NullElement::simple_action(Packet *p)
{
  return p;
}

PushNullElement::PushNullElement()
{
}

PushNullElement::~PushNullElement()
{
}

  void
PushNullElement::push(int, Packet *p)
{
  //while(_lk.compare_swap(0, 1) != 0);
  // do something here
  //_lk.swap(0);
  output(0).push(p);
}


BatchLockingPushNullElement::BatchLockingPushNullElement()
{
  _ticket = 0;
  _nowserving = 0;
}

int 

BatchLockingPushNullElement::configure(Vector<String> &conf, ErrorHandler *errh){
  _q = new Packet**[router()->master()->nthreads()];
  _occupancy = new uint8_t[router()->master()->nthreads()];
  for(int i = 0; i < router()->master()->nthreads(); i++){
    _q[i] = new Packet*[128];
    _occupancy[i] = 0; 
  }
  return 0;
}

BatchLockingPushNullElement::~BatchLockingPushNullElement()
{
}

  void
BatchLockingPushNullElement::push(int, Packet *p)
{
  int BATCHSIZE = 128;
  uint8_t curthread = p->_parent_thread;
  _q[p->_parent_thread][_occupancy[curthread]] = p;
  _occupancy[curthread]++;

  if((uint8_t) (_occupancy[curthread]) == (uint8_t) BATCHSIZE){
    uint32_t myticket = _ticket.fetch_and_add(1);
    while(myticket != _nowserving);
    //Do something!
    _nowserving++;

    for(int i = 0; i < BATCHSIZE; i++){
      output(0).push(_q[curthread][i]);
    }
    _occupancy[curthread] = 0;
  }
}



LockingPushNullElement::LockingPushNullElement()
{
  _ticket = 0;
  _nowserving = 0;
}

LockingPushNullElement::~LockingPushNullElement()
{
}

  void
LockingPushNullElement::push(int, Packet *p)
{
  /*uint32_t myticket = _ticket.fetch_and_add(1);
    while(myticket != _nowserving);
  //Do something!
  _nowserving++;*/
  while(_ticket.compare_swap(0, 1) != 0);
  _ticket.swap(0);

  output(0).push(p);
}

PullNullElement::PullNullElement()
{
}

PullNullElement::~PullNullElement()
{
}

  Packet *
PullNullElement::pull(int)
{
  Packet * p = input(0).pull();
  //while(_lk.compare_swap(0, 1) != 0);
  //_lk.swap(0);
  return p;
}

  CLICK_ENDDECLS
  EXPORT_ELEMENT(NullElement)
  EXPORT_ELEMENT(PushNullElement)
  EXPORT_ELEMENT(PullNullElement)
  EXPORT_ELEMENT(LockingPushNullElement)
  EXPORT_ELEMENT(BatchLockingPushNullElement)
  ELEMENT_MT_SAFE(BatchLockingPushNullElement)
  ELEMENT_MT_SAFE(LockingPushNullElement)
  ELEMENT_MT_SAFE(NullElement)
  ELEMENT_MT_SAFE(PushNullElement)
ELEMENT_MT_SAFE(PullNullElement)
