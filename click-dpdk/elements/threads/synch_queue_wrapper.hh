// -*- c-basic-offset: 4 -*-
#ifndef CLICK_SYNCHQUEUEWRAPPER_HH
#define CLICK_SYNCHQUEUEWRAPPER_HH
#include "../standard/fullnotequeue.hh"
CLICK_DECLS

/*
=c

SynchQueueWrapper
SynchQueueWrapper(CAPACITY)

=s storage

stores packets in a FIFO queue

=d

Stores incoming packets in a first-in-first-out queue.
Drops incoming packets if the queue already holds CAPACITY packets.
The default for CAPACITY is 1000.

This variant of the default Queue is (should be) completely thread safe, in
that it supports multiple concurrent pushers and pullers.  In all respects
other than thread safety it behaves just like Queue, and like Queue it has
non-full and non-empty notifiers.

=h length read-only

Returns the current number of packets in the queue.

=h highwater_length read-only

Returns the maximum number of packets that have ever been in the queue at once.

=h capacity read/write

Returns or sets the queue's capacity.

=h drops read-only

Returns the number of packets dropped by the queue so far.

=h reset_counts write-only

When written, resets the C<drops> and C<highwater_length> counters.

=h reset write-only

When written, drops all packets in the queue.

=a Queue, SimpleQueue, NotifierQueue, MixedQueue, FrontDropQueue */

class SynchQueueWrapper : public FullNoteQueue { public:

    SynchQueueWrapper();
    ~SynchQueueWrapper();

    //virtual const char *port_count() const    { return "1/2"; }
    //virtual const char *processing() const    { return "h/hl"; }


    virtual const char *port_count() const    { return "2/3"; }
    virtual const char *processing() const    { return "hh/hhl"; }



    const char *class_name() const		{ return "SynchQueueWrapper"; }
    void *cast(const char *);

    int initialize(ErrorHandler* errh);

    int configure(Vector<String> &conf, ErrorHandler *errh);
    int live_reconfigure(Vector<String> &conf, ErrorHandler *errh);

    void push(int port, Packet *);
    Packet *pull(int port);

  private:

    atomic_uint32_t _xhead;
    atomic_uint32_t _xtail;

    //Updated on push to wrapped element
    Storage::index_type wrap_h, wrap_t, wrap_nt;

    uint64_t cur_threadid;
    uint16_t cur_thread_timesheld;
};

CLICK_ENDDECLS
#endif
