#ifndef CLICK_BlindBoxFilter_HH
#define CLICK_BlindBoxFilter_HH
#include <click/element.hh>
#include <click/task.hh>
#include <click/notifier.hh>
#include <iostream>
#include <click/signaturetable.hh>
CLICK_DECLS

class BlindBoxFilter : public Element { 
  
  public:

  BlindBoxFilter();
  ~BlindBoxFilter();

  const char *class_name() const		{ return "BlindBoxFilter"; }
  void *cast(const char *);
  const char *port_count() const		{ return PORTS_1_1; }
  const char *processing() const {  return PUSH;  }

  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);
  bool can_live_reconfigure() const		{ return false; }
  void cleanup(CleanupStage);

  void push(int port, Packet* p);

  protected:
    GlobalSignatureTable* gt;
    ThreadLocalSignatureTable* st;
    char* SIGNATURES[10000];
};

CLICK_ENDDECLS
#endif
