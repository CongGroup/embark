#ifndef CLICK_MBArkDPI_HH
#define CLICK_MBArkDPI_HH
#include <click/ip6address.hh>
#include <click/ipaddress.hh>
#include <click/vector.hh>
#include <click/element.hh>
#include <click/fromfile.hh>
#include <click/tokenizer.hh>
#include <click/aes.hh>

CLICK_DECLS

class MBArkDPI : public Element {

 public:
  MBArkDPI();
  ~MBArkDPI();

  int configure(Vector<String> &, ErrorHandler *);

  const char *class_name() const		{ return "MBArkDPI"; }
  const char *port_count() const		{ return "1/2"; }
  
  const char *processing() const	{ return PUSH; }
  void push(int, Packet* p);
  //  Packet *simple_action(Packet *p_in);

  private:
  MY_AES_KEY key;
  
  char mystrings[1500 * 32];
  content_string csstrings[1500];
  cs_list_node nodes[1500];

  cs_list_node* freed;
  cs_list_node* allocd;

  bool dividers[256];
};

CLICK_ENDDECLS
#endif
