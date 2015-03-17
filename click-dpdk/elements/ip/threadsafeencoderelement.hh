#ifndef CLICK_ThreadSafeEncoderElement_HH
#define CLICK_ThreadSafeEncoderElement_HH
#include <click/element.hh>
#include <click/glue.hh>
#include <click/atomic.hh>
#include "packetstore.hh"
#include <click/mcs_lock.h>
#include "index.hh"

#define MAXINSERTS 5 
CLICK_DECLS

#ifndef MINIMA 
#define MINIMA(a,b)  (((a)<(b))?(a):(b)) 
#endif

#ifndef MAXIMA 
#define MAXIMA(a,b)  (((a)>(b))?(a):(b))
#endif

#ifndef ENCODEINFO
#define ENCODEINFO

/*
 */
class Footprint{
	public:
	uint64_t fprint;
	uint16_t left;
	uint16_t right;
	uint16_t offset;
};

class MatchInfo{
	public:
	uint64_t fprint;
	uint64_t uuid;
	uint16_t offset;
};




class Encode{
	public:
	uint64_t uuid; // pkt with whose uuid it is encoded
	uint16_t offset; // offset of that packet
	uint16_t length; // length of match
	uint16_t my_offset; // offset of my packet.
};
#endif

class ThreadSafeEncoderElement : public Element { 
public:

  ThreadSafeEncoderElement();
  
  ~ThreadSafeEncoderElement();
  
  const char *class_name() const		{ return "ThreadSafeEncoderElement"; }
  const char *port_count() const		{ return "1/0-1"; }
  const char *processing() const		{ return PUSH; }
  int configure(Vector<String> &, ErrorHandler *);
 /* Setup the prime number - "user gives."
    Setup the siglen = The length of the window over which hash is computed
       "user gives."
    Setup the sigmask = beta - "Process user input."
    Setup the hashmask - the length of which has to be handled carefully (See Neil's code). - "Process user input."
  Setup the table[256] with values. "Just make a function call."
*/ 
  //void add_handlers();

  void push(int, Packet *);

  private:
  uint64_t table[256];
  uint64_t current_uuid;
  uint64_t max_packets;
  uint64_t prime;
  uint64_t hashmask; /* 60 bits  = M (with which mod is taken)*/
  //atomic_uint32_t _lk;
  mcslock_t _lk;
  
  unsigned short siglen;
  uint64_t sigmask;
  Packet *computeHash(Packet *p);
  uint64_t hash_getmaskoflength(unsigned int new_masklen);
  void setupTable();
  void inline hash_advanceHash(uint64_t *pHash, const unsigned char *, const unsigned char **, const unsigned char **);
  inline unsigned char sigmask_isAGoodHash(uint64_t hash); 
  hashTable *index;
  packetStore *store;
  Footprint represent[MAXINSERTS];
  MatchInfo matchinfo[MAXINSERTS];

  int inline hash_testAndInsert(uint64_t hash, pkt *pPkt, unsigned short offset, unsigned short *pRedundantLeftBound, int inserts);
  unsigned short walk_right(const unsigned char *pStore, const unsigned char *pHere, unsigned short redundancy_limit);
  unsigned short walk_left(const unsigned char *pStore, const unsigned char *pHere, unsigned short redundancy_limit);
void collision_detect(hashEntry *pHashEntry,
		pkt *pPkt, unsigned short offset, 
		unsigned short *pleftbound, int inserts);
inline void hash_testAndNoCollide(uint64_t hash, unsigned short offset);
};
CLICK_ENDDECLS
#endif
