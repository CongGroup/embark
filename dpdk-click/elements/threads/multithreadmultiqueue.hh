// -*- c-basic-offset: 4 -*-
#ifndef CLICK_MultiThreadMultiQueue_HH
#define CLICK_MultiThreadMultiQueue_HH
#include <iostream>
#include <click/element.hh>
#include <click/standard/storage.hh>
CLICK_DECLS

class MultiThreadMultiQueue : public Element { public:


  typedef uint32_t index_type;
  typedef int32_t signed_index_type;



  MultiThreadMultiQueue();
  ~MultiThreadMultiQueue();

  int drops() const				{ return _drops; }
  int highwater_length() const		{ return _highwater_length; }

  inline bool enq(Packet*);
  inline Packet* deq(uint8_t tid);

  // to be used with care
  //Packet* packet(int i) const			{ return _q[i]; }
  void reset();				// NB: does not do notification

  const char *class_name() const		{ return "MultiThreadMultiQueue"; }
  virtual const char *port_count() const		{ return "1/-"; }
  const char *processing() const		{ return "h/l"; }
  void* cast(const char*);


  int size(index_type head, index_type tail) const;
  int configure(Vector<String>&, ErrorHandler*);
  int initialize(ErrorHandler*);
  void cleanup(CleanupStage);
  bool can_live_reconfigure() const		{ return false; }
  int live_reconfigure(Vector<String>&, ErrorHandler*);
  void take_state(Element*, ErrorHandler*);
  void add_handlers();

  static uint8_t get_cur_threadid();
  void push(int port, Packet*);
  Packet* pull(int port);

  protected:

  //Packet ** _packetdata;
  Packet*** _q;
  volatile int _drops;
  int _highwater_length;

  friend class MixedQueue;
  friend class TokenQueue;
  friend class InOrderQueue;
  friend class ECNQueue;

  static String read_handler(Element*, void*);
  static int write_handler(const String&, Element*, void*, ErrorHandler*);

  index_type _capacity;
  index_type* _heads;
  index_type* _tails;
  uint8_t _nthreads;

  index_type next_i(index_type i) const {
    return (i!=_capacity ? i+1 : 0);
  }
  index_type prev_i(index_type i) const {
    return (i!=0 ? i-1 : _capacity);
  }


};


inline int
MultiThreadMultiQueue::size(index_type head, index_type tail) const
{
  index_type x = tail - head;
  return (signed_index_type(x) >= 0 ? x : _capacity + x + 1);
}

  inline bool
MultiThreadMultiQueue::enq(Packet *p)
{
  assert(p);
  uint8_t tid = get_cur_threadid();

  Storage::index_type h = _heads[tid], t = _tails[tid], nt = next_i(t);
  if (nt != h) {
    _q[tid][t] = p;
    //packet_memory_barrier(_q[t], _tail);
    _tails[tid] = nt;
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


inline Packet *
MultiThreadMultiQueue::deq(uint8_t tid)
{
  Storage::index_type h = _heads[tid], t = _tails[tid];
  //std::cout << this << "tid " << ((uint32_t) tid) << " h " << h << "t " << t << std::endl;
  if (h != t) {
    Packet *p = _q[tid][h];
    //packet_memory_barrier(_q[h], _head);
    _heads[tid] = next_i(h);
    assert(p);
    return p;
  } else
    return 0;
}



CLICK_ENDDECLS
#endif
