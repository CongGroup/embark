#ifndef CLICK_PACKETSTORE_HH
#define CLICK_PACKETSTORE_HH
#include <click/ipaddress.hh>
#include <clicknet/ip.h>
CLICK_DECLS
class hashEntry;
class hashTable;
class pkt {
	public:
	pkt(const unsigned char *data, IPAddress *src, IPAddress *dst, uint16_t sport, uint16_t dport, uint8_t proto, unsigned short length);
	pkt();
	~pkt();
	//enum t_valid {false, true} valid_T;
		//enum t_valid valid;
	unsigned char valid;	
	unsigned short _length;
	//pkt *previous;
	//pkt *next;
	uint64_t myposition;
	unsigned char _packetData[1500];
/* confusing!! aargh. */
	IPAddress *source;
	IPAddress *destination;
	uint16_t srcport;
	uint16_t destport;
	uint8_t protocol;
	//hashEntry *indexOfMe;

	/* make private at some point - Maybe at the constructor
	 level is good enough*/
	unsigned short length(){
		return _length;
	}
	void set_length(unsigned short length){
		_length = length;
	}
	const unsigned char *packetData(){
		return _packetData;
	}
/* The thing to note is that the *packetData is allocated on the 
   heap by calling the pkt constructor and has to be de-allocated
   at the destructor... delete is the keyword 
   For now I have allocated it with the pkt
 */
	void regen_pkt(const unsigned char *data, IPAddress *src, IPAddress *dst, uint16_t sport, uint16_t dport, uint8_t proto, unsigned short length);

	void set_invalid();
	unsigned char validity();
};

/*implements a circular buffer - have a max I guess and some flag for
  when max reached */
class packetStore{
	public:
		packetStore(uint64_t store_size);
		~packetStore();
		//enum t_valid {false, true};
		uint64_t max_packets;
		uint64_t current_pos;
		pkt **store;
		/* circular updation of current_pos*/
		unsigned char max_reached;
		void removePacket();
		pkt *addPacket(const unsigned char * payload_start, IPAddress *src, IPAddress *dst,uint16_t sport, uint16_t dport, uint8_t proto, unsigned short length);
		//pkt *addPacket(const unsigned char * payload_start, IPAddress *src, IPAddress *dst,uint16_t sport, uint16_t dport, uint8_t proto, unsigned short length,hashTable *theIndex);
		inline pkt *getPacket(uint64_t uuid, uint64_t current_uuid ){
			if(uuid < (current_uuid - max_packets)){
				return NULL;
			}
			else{
				return store[((uuid-max_packets-2)%max_packets)];
			}
		}
		inline unsigned short getvalid(uint64_t uuid, uint64_t current_uuid){
			if(uuid < (current_uuid - max_packets)){
				return 0;
			}
			else{
				return 1;
			}
		}
};

CLICK_ENDDECLS
#endif
