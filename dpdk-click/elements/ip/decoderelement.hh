#ifndef CLICK_DECODERELEMENT_HH
#define CLICK_DECODERELEMENT_HH
#include <click/element.hh>
#include <click/glue.hh>
#include <click/atomic.hh>
#include "packetstore.hh"

#define MAXINSERTS 5 
CLICK_DECLS

#ifndef MINIMA 
#define MINIMA(a,b)  (((a)<(b))?(a):(b)) 
#endif

#ifndef MAXIMA 
#define MAXIMA(a,b)  (((a)>(b))?(a):(b))
#endif


/*
 */
#ifndef ENCODEINFO
#define ENCODEINFO
class Encode{
	public:
	uint64_t uuid; // pkt with whose uuid it is encoded
	uint16_t offset; // offset of that packet
	uint16_t length; // length of match
	uint16_t my_offset; // offset of my packet.
};
#endif

class DecoderElement : public Element { 
public:

  DecoderElement();
  
  ~DecoderElement();
  
  const char *class_name() const		{ return "DecoderElement"; }
  const char *port_count() const		{ return "1/0-1"; }
  const char *processing() const		{ return PUSH; }
  int configure(Vector<String> &, ErrorHandler *);
  void push(int, Packet *);

  private:
  uint64_t current_uuid;
  uint64_t max_packets;
  uint64_t prime;
  uint64_t hashmask; /* 60 bits  = M (with which mod is taken)*/
  unsigned short siglen;
  uint64_t sigmask;
  uint64_t packetcount;
  uint64_t totalbits;
  uint64_t redundant;
  Packet *decodePacket(Packet *p);
  uint64_t hash_getmaskoflength(unsigned int new_masklen);
  packetStore *receiver_store; // for the receiver side
  uint64_t receiver_current_uuid; // for the receiver side
  unsigned char decoded_data[1500];
  Encode encodeobjs[MAXINSERTS];
};
CLICK_ENDDECLS
#endif
