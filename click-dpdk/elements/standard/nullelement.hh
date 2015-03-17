#ifndef CLICK_NULLELEMENT_HH
#define CLICK_NULLELEMENT_HH
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

class NullElement : public Element { public:

  NullElement();
  ~NullElement();

  const char *class_name() const	{ return "Null"; }
  const char *port_count() const	{ return PORTS_1_1; }

  Packet *simple_action(Packet *);

};

/*
=c
PushNull

=s basictransfer
push-only null element

=d
Responds to each pushed packet by pushing it unchanged out its first output.

=a
Null, PullNull
*/

class PushNullElement : public Element { public:

  PushNullElement();
  ~PushNullElement();

  const char *class_name() const	{ return "PushNull"; }
  const char *port_count() const	{ return PORTS_1_1; }
  const char *processing() const	{ return PUSH; }

  void push(int, Packet *);

};

class LockingPushNullElement : public Element { public:

  LockingPushNullElement();
  ~LockingPushNullElement();

  const char *class_name() const	{ return "LockingPushNull"; }
  const char *port_count() const	{ return PORTS_1_1; }
  const char *processing() const	{ return PUSH; }

  void push(int, Packet *);

  atomic_uint32_t _ticket;
  atomic_uint32_t _nowserving;
};


class BatchLockingPushNullElement : public Element { public:

  BatchLockingPushNullElement();
  ~BatchLockingPushNullElement();

  int configure(Vector<String> &conf, ErrorHandler *errh);
  const char *class_name() const	{ return "BatchLockingPushNull"; }
  const char *port_count() const	{ return PORTS_1_1; }
  const char *processing() const	{ return PUSH; }

  void push(int, Packet *);

  Packet*** _q;
  uint8_t* _occupancy;
  atomic_uint32_t _ticket;
  atomic_uint32_t _nowserving;
};




/*
=c
PullNull

=s basictransfer
pull-only null element

=d
Responds to each pull request by pulling a packet from its input and returning
that packet unchanged.

=a
Null, PushNull */

class PullNullElement : public Element { public:

  PullNullElement();
  ~PullNullElement();

  const char *class_name() const	{ return "PullNull"; }
  const char *port_count() const	{ return PORTS_1_1; }
  const char *processing() const	{ return PULL; }

  Packet *pull(int);

};

CLICK_ENDDECLS
#endif
