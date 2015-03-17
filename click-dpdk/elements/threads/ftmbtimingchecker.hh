// -*- c-basic-offset: 4 -*-
#ifndef CLICK_FTMBTimingChecker_HH
#define CLICK_FTMBTimingChecker_HH
#include <click/element.hh>
#include <click/task.hh>
#include <click/standard/storage.hh>
CLICK_DECLS

class FTMBTimingChecker : public Element { public:


  typedef uint32_t index_type;
  typedef int32_t signed_index_type;



  FTMBTimingChecker();
  ~FTMBTimingChecker();

  int drops() const				{ return _drops; }
  int highwater_length() const		{ return _highwater_length; }


  // to be used with care
  //Packet* packet(int i) const			{ return _q[i]; }
  void reset();				// NB: does not do notification

  const char *class_name() const		{ return "FTMBTimingChecker"; }
  virtual const char *port_count() const		{ return "1/1"; }
  const char *processing() const		{ return "h/h"; }
  void* cast(const char*);

  virtual bool run_task(Task * t);
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
  //Packet* pull(int port);

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

  Timer _timer;
  Task _mytask;

  index_type next_i(index_type i) const {
    return (i!=_capacity ? i+1 : 0);
  }
  index_type prev_i(index_type i) const {
    return (i!=0 ? i-1 : _capacity);
  }

  private:
  volatile bool _pause_compare;

};


inline int
FTMBTimingChecker::size(index_type head, index_type tail) const
{
  index_type x = tail - head;
  return (signed_index_type(x) >= 0 ? x : _capacity + x + 1);
}


CLICK_ENDDECLS
#endif
