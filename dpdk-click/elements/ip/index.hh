#ifndef CLICK_INDEX_HH
#define CLICK_INDEX_HH
/*Header files*/
#include "packetstore.hh"
CLICK_DECLS

/* the index table is an array of pointers to lists of hash 
   entries */
class hashEntry{
	/*set identifiers - mark out public and private properly*/
	public:
  	uint64_t _uuid;
  	uint64_t _fingerprint;
  /* can describe start & max_extent from pHoldingPacket and offset*/ 
  	unsigned short _offset; 
	hashEntry();
	hashEntry(uint64_t fingerprint,uint64_t uuid, unsigned short offset);
	~hashEntry();
	//pkt *_pHoldingPacket;
	inline uint64_t fingerprint(){
		return _fingerprint;
	}
	inline void set_fingerprint(uint64_t fingerprint){
		_fingerprint = fingerprint;
	}
	inline unsigned short offset(){
		return _offset;
	}
	inline void set_offset(unsigned short offset){
		_offset = offset;
	}

  	//hashEntry *idxNext;  // next in the hashtable bucket
  	//hashEntry *pktNext;  // next in the indices for this pkt.
  	//hashEntry *pktPrev;  // previous in the indices for this pkt
	//might do away with function calling overhead?

	//REWRITE THESE FUNCTIONS.. CHECK WHERE - walk_left, walk_right
	//Sanity checking for the uuid expiry
	//inline const unsigned char *start(){
	  //return (_pHoldingPacket->packetData()+_offset);
  	//}
	//inline unsigned short max_extent(){
	//	return (_pHoldingPacket->length()-_offset);
	//void addPacketAssociation(pkt *holdingPacket);
	//void addPacketAssociation(uint64_t uuid);
	/* Function for adding in
	   yourself at head of linked list 
	   and update the packet association bothways as you are 
	 at the head of the list*/
	//void removePacketAssociation(); //set UUID = 0
	/* get out of the linked list cleanly*/
	//void check_valid(){

};

class hashTable {
	
	public:
	inline uint32_t indexForFingerprint(const uint64_t fingerprint);
	uint32_t _tablesize;
	uint32_t _modsize;
	uint32_t holding_counter;
	uint32_t failure_counter;
	hashEntry *index; // this is initialized in the constructor.. should be terminated at the destructor.
		hashTable(uint64_t tsize);
		~hashTable();
	hashEntry *lookup(const uint64_t fingerprint,  uint64_t max_packets, uint64_t current_uuid);
	//hashEntry *lookupAndInsert(const uint64_t fingerprint, unsigned short offset, pkt *pHoldingPacket);
	hashEntry *insert(const uint64_t fingerprint, unsigned short offset, uint64_t uuid,  uint64_t max_packets, uint64_t current_uuid);
	//hashEntry *insert(const uint64_t fingerprint, unsigned short offset, pkt *pHoldingPacket);
	//hashEntry *replace(hashEntry *entry, unsigned short offset,pkt *holdingPacket); 
	hashEntry *replace(hashEntry *entry, unsigned short offset,uint64_t uuid); 
	//int freeEntry(hashEntry *entry);
	void  invalidateFpIfInRange(const uint64_t fingerprint, uint16_t left, uint16_t right,uint64_t max_packets,uint64_t current_uuid);

	void  reassignFpIfInRange(const uint64_t fingerprint, uint16_t left, uint16_t right, uint64_t new_uuid, uint16_t my_offset, uint16_t match_offset, uint64_t max_packets,uint64_t current_uuid);
};

CLICK_ENDDECLS 
#endif
