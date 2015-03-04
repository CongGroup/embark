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
#include "threadfanner.hh"
CLICK_DECLS



ThreadFanner::ThreadFanner()
{
}

ThreadFanner::~ThreadFanner()
{
}

void
ThreadFanner::push(int, Packet *p)
{
  //while(_lk.compare_swap(0, 1) != 0);
  //_lk.swap(0);
  output(p->_parent_thread).push(p);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(ThreadFanner)
ELEMENT_MT_SAFE(ThreadFanner)
