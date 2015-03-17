// -*- c-basic-offset: 4 -*-
#ifndef CLICK_COREQUEUE_HH
#define CLICK_COREQUEUE_HH
#include <click/element.hh>
//#include <click/standard/storage.hh>
CLICK_DECLS

/*
=c

SimpleQueue
SimpleQueue(CAPACITY)

=s storage

stores packets in a FIFO queue

=d

Stores incoming packets in a first-in-first-out queue.
Drops incoming packets if the queue already holds CAPACITY packets.
The default for CAPACITY is 1000.

B<Multithreaded Click note:> SimpleQueue is designed to be used in an
environment with at most one concurrent pusher and at most one concurrent
puller.  Thus, at most one thread pushes to the SimpleQueue at a time and at
most one thread pulls from the SimpleQueue at a time.  Different threads can
push to and pull from the SimpleQueue concurrently, however.  See
ThreadSafeQueue for a queue that can support multiple concurrent pushers and
pullers.

=n

The Queue and NotifierQueue elements act like SimpleQueue, but additionally
notify interested parties when they change state (from nonempty to empty or
vice versa, and/or from nonfull to full or vice versa).

=h length read-only

Returns the current number of packets in the queue.

=h highwater_length read-only

Returns the maximum number of packets that have ever been in the queue at once.

=h capacity read/write

Returns or sets the queue's capacity.

=h drops read-only

Returns the number of packets dropped by the queue so far.  Dropped packets
are emitted on output 1 if output 1 exists.

=h reset_counts write-only

When written, resets the C<drops> and C<highwater_length> counters.

=h reset write-only

When written, drops all packets in the queue.

=a Queue, NotifierQueue, MixedQueue, RED, FrontDropQueue, ThreadSafeQueue */

class CoreQueue : public Element { public:

    CoreQueue();
    ~CoreQueue();

    int drops() const				{ return _drops; }
    int highwater_length() const		{ return _highwater_length; }

    //    inline bool enq(Packet*);
    // inline void lifo_enq(Packet*);
    //inline Packet* deq();    

    inline bool simple_enq(Packet*);
    inline Packet*  simple_deq();

    // to be used with care
    // Packet* packet(int i) const			{ return _q[i]; }
    //void reset();				// NB: does not do notification
    
    const char *class_name() const		{ return "CoreQueue"; }
    const char *port_count() const		{ return PORTS_1_1; } //{ return "1/1"; }
    const char *processing() const		{ return PUSH_TO_PULL; }//{ return "h/l"; }
   

    int configure(Vector<String>&, ErrorHandler*);
    int initialize(ErrorHandler*);
    void cleanup(CleanupStage);
    
    void add_handlers();

    void push(int port, Packet*);
    Packet* pull(int port);

private:
    //simple storage 
    unsigned _counter;
    Packet* pkt_queue[100];
    unsigned size;

    unsigned int _drops;
    int _highwater_length;



protected:


    static String read_handler(Element*, void*);
    static int write_handler(const String&, Element*, void*, ErrorHandler*);

};

inline bool
CoreQueue::simple_enq(Packet *p)
{
    //assert(p);
    if (unlikely(_counter == 99)) {
	p->kill();
	_drops++;
	return false;
    }
    _counter++;
    pkt_queue[_counter] = p;
    return true;
}
/*
inline bool
CoreQueue::enq(Packet *p)
{
    assert(p);
    Storage::index_type h = _head, t = _tail, nt = next_i(t);
    if (nt != h) {
	_q[t] = p;
	packet_memory_barrier(_q[t], _tail);
	_tail = nt;
	int s = size(h, nt);
	if (s > _highwater_length)
	    _highwater_length = s;
	return true;
    } else {
	p->kill();
	_drops++;
	return false;
    }
}

inline void
CoreQueue::lifo_enq(Packet *p)
{
    // XXX NB: significantly more dangerous in a multithreaded environment
    // than plain (FIFO) enq().
    assert(p);
    Storage::index_type h = _head, t = _tail, ph = prev_i(h);
    if (ph == t) {
	t = prev_i(t);
	_q[t]->kill();
	_tail = t;
    }
    _q[ph] = p;
    packet_memory_barrier(_q[ph], _head);
    _head = ph;
}
*/
inline Packet *
CoreQueue::simple_deq()
{
    if (likely(_counter > 0)) {
	Packet* p = pkt_queue[_counter];
	_counter--;
	//assert(p);
	return p;
    }
    else
	return NULL;
}
/*
inline Packet *
CoreQueue::deq()
{
    Storage::index_type h = _head, t = _tail;
    if (h != t) {
	Packet *p = _q[h];
	packet_memory_barrier(_q[h], _head);
	_head = next_i(h);
	assert(p);
	return p;
    } else
	return 0;
}
*/

CLICK_ENDDECLS
#endif
