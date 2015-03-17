#include <click/config.h>
#include "index.hh"
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/glue.hh>
#include "packetstore.hh"
CLICK_DECLS

hashEntry::hashEntry(uint64_t fingerprint, uint64_t uuid, unsigned short offset){
	_fingerprint = fingerprint;
	_offset = offset;
	_uuid = uuid;
}

hashEntry::~hashEntry()
{
}
hashEntry::hashEntry()
{
}

inline uint32_t hashTable::indexForFingerprint(const uint64_t fingerprint)
{
	return 2*(((fingerprint >> 8) ^ fingerprint) % _modsize);
}

hashTable::hashTable(uint64_t tsize)
{
	holding_counter = 0;
	/*uint64_t shiftme = tsize;
	uint64_t tempsize = 0x080000000U;
	while((shiftme & 0x080000000U) == 0){
		shiftme<<= 1;
		tempsize>>= 1;
	}
	//tempsize <<= 1; // check this step? did neil do this??
	//tempsize <<= 1;
		tempsize>>= 1;
		tempsize>>= 2;*/
	_modsize = (tsize+1)*2 ;
	_tablesize = (_modsize)*2 ;
	click_chatter("%lu\n",_tablesize);
	index = new hashEntry[_tablesize];
	for(int i = 0; i < _tablesize; i++){
		index[i]._uuid = 0;
	}
	click_chatter(" working\n");
}

hashTable::~hashTable()
{
	delete[] index;
}

hashEntry * hashTable::lookup(const uint64_t fingerprint, uint64_t max_packets, uint64_t current_uuid)
{
	uint32_t idx = indexForFingerprint(fingerprint);
	if(index[idx]._uuid != 0){
		if(index[idx]._fingerprint == fingerprint && index[idx]._uuid > (current_uuid - max_packets) ){
			return &index[idx];
		}
	}
	idx++;
	if(index[idx]._uuid != 0){
		if(index[idx]._fingerprint == fingerprint && index[idx]._uuid > (current_uuid - max_packets)){
			return &index[idx];
		}
	}
	return NULL;
}

void  hashTable::invalidateFpIfInRange(const uint64_t fingerprint, uint16_t left, uint16_t right, uint64_t max_packets,uint64_t current_uuid){

	uint32_t idx = indexForFingerprint(fingerprint);
	if(index[idx]._uuid == current_uuid && index[idx]._fingerprint == fingerprint  
		 && index[idx]._offset >= left
		 && index[idx]._offset <= right)
		index[idx]._uuid = 0;
	idx ++;
	if(index[idx]._uuid == current_uuid && index[idx]._fingerprint == fingerprint  
		 && index[idx]._offset >= left
		 && index[idx]._offset <= right)
 	index[idx]._uuid = 0;

}

void  hashTable::reassignFpIfInRange(const uint64_t fingerprint, uint16_t left, uint16_t right, uint64_t new_uuid, uint16_t my_offset, uint16_t match_offset, uint64_t max_packets,uint64_t current_uuid){

	uint32_t idx = indexForFingerprint(fingerprint);
	// essentially these fingerprints are pointing to current-uuid? in this case??
	if(index[idx]._uuid == current_uuid && index[idx]._fingerprint == fingerprint  
				 && index[idx]._offset >= left
				 && index[idx]._offset <= right){
		index[idx]._uuid = new_uuid;
		index[idx]._offset = match_offset-my_offset + index[idx]._offset;
	}
	idx ++;
	if(index[idx]._uuid == current_uuid && index[idx]._fingerprint == fingerprint 
				 && index[idx]._offset >= left
				 && index[idx]._offset <= right){
		index[idx]._uuid = new_uuid;
		index[idx]._offset = match_offset- my_offset + index[idx]._offset;
	}

}


hashEntry * hashTable::insert(const uint64_t fingerprint, unsigned short offset, uint64_t uuid,  uint64_t max_packets, uint64_t current_uuid)
{
	uint32_t idx = indexForFingerprint(fingerprint);
	//invalid entry
	if(index[idx]._uuid ==0 || index[idx]._uuid < (current_uuid - max_packets)){

	//click_chatter("index value %d\n",idx);
		index[idx]._fingerprint = fingerprint;
		index[idx]._offset = offset;
		index[idx]._uuid = uuid;
	holding_counter++;
		return NULL;
	}
	idx++;
	if(index[idx]._uuid ==0 || index[idx]._uuid < (current_uuid - max_packets)){

	//click_chatter("index value %d\n",idx);
		index[idx]._fingerprint = fingerprint;
		index[idx]._offset = offset;
		index[idx]._uuid = uuid;
	holding_counter++;
		return NULL;
	}
	failure_counter++;
	//if(pHashEntry->idxNext == NULL){
		//click_chatter("could be fatal\n");
	//}
	//pHashEntry->addPacketAssociation(uuid);
	return NULL;
}

/*hashEntry hashTable::lookupAndInsert(const uint64_t fingerprint, unsigned short offset, pkt *pHoldingPacket)
{
	
}*/

hashEntry * hashTable::replace(hashEntry *entry, unsigned short offset,uint64_t uuid)
{
	//entry->removePacketAssociation();
	entry->_uuid = uuid;
	entry->_offset = offset;
	//entry->addPacketAssociation(holdingPacket);
	return entry;
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(PacketStore)
ELEMENT_PROVIDES(HashIndex)
