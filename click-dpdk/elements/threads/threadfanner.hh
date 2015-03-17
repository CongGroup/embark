#ifndef CLICK_THREADFANNER_HH
#define CLICK_THREADFANNER_HH
#include <click/element.hh>
CLICK_DECLS

/*
=c
Null

=s basictransfer
null element: passes packets unchanged

=d
Just passes packets along without doing anything else.

=a
PushNull, PullNull
*/

class ThreadFanner : public Element { public:

  ThreadFanner();
  ~ThreadFanner();

  const char *class_name() const	{ return "ThreadFanner"; }
  const char *port_count() const	{ return "1/-"; }
  const char *processing() const	{ return PUSH; }

  void push(int, Packet *);

};

CLICK_ENDDECLS
#endif
