#include <click/config.h>
#include "packetstore.hh"
#include "index.hh"
#include <click/confparse.hh>
#include <click/glue.hh>
#define false 0
#define true 1
CLICK_DECLS

pkt::pkt(){
  valid = true;
	_length = 0;
	//memcpy(_packetData,data,length);
	source = 0;
	destination = 0;
	srcport = 0;
	destport =  0;
	protocol = 0;
	//indexOfMe = NULL;

}


pkt::pkt(const unsigned char *data, IPAddress *src, IPAddress *dst, uint16_t sport, uint16_t dport, uint8_t proto, unsigned short length){
	valid = true;
	_length = length;
	memcpy(_packetData,data,length);
  if(source) delete source;
	source = src;
  if(destination) delete destination;
	destination = dst;
	srcport = sport;
	destport = dport;
	protocol = proto;
	//indexOfMe = NULL;
}

pkt::~pkt()
{
}

void pkt::regen_pkt(const unsigned char *data, IPAddress *src, IPAddress *dst, uint16_t sport, uint16_t dport, uint8_t proto, unsigned short length){
	valid = true;
	_length = length;
	memcpy(_packetData,data,length);
  if(source) delete source;
  source = src;
  if(destination) delete destination;
  destination = dst;
	srcport = sport;
	destport = dport;
	protocol = proto;
	//indexOfMe = NULL;
}

void pkt::set_invalid()
{
	valid = false;
	//clean the fingerprints pointing here.
}

unsigned char pkt::validity()
{
	return (valid);
}

packetStore::packetStore(uint64_t store_size)
{
	max_packets = store_size / sizeof(pkt);
	//click_chatter("max packets: %llu\n",max_packets);
	current_pos = 0;
	max_reached = false;
	store = new pkt*[max_packets];
	// initialize all packets of store.
	for(int i = 0; i < max_packets ; i ++){
		store[i] = new pkt(); // just initialization. all the
		// memory allocation is done beforehand.
	}
}

packetStore::~packetStore()
{
	delete[] store;
}

pkt * packetStore::addPacket(const unsigned char * payload_start, IPAddress *src, IPAddress *dst,uint16_t sport, uint16_t dport, uint8_t proto, unsigned short length)
{
	pkt *newpacket;
	if(max_reached){
		newpacket = store[current_pos];
		newpacket->regen_pkt(payload_start,src,dst,sport,dport,proto,length);
		
		//circulate.. clear one entry etc etc.
		//go round in the buffer. regen_pkt
		//mark the next packet invalid...
		//clear the fingerprints pointing here..
		current_pos = (current_pos+1)% max_packets;
		//pkt *delpacket = store[current_pos];
		/*hashEntry *pHE, *pHE2;
		pHE = delpacket->indexOfMe;
		if(pHE!=NULL){
			while(pHE->pktNext != NULL){
				pHE2 = pHE->pktNext;
				theIndex->freeEntry(pHE);
				pHE = pHE2;
			}
			theIndex->freeEntry(pHE);
		}*/
		//delpacket->set_invalid();
	}
	else{
		newpacket = store[current_pos];
		newpacket->regen_pkt(payload_start,src,dst,sport,dport,proto,length);
		//newpacket = new pkt(payload_start, src, dst, sport, dport, proto, length);
		//store[current_pos] = newpacket;

		newpacket->myposition = current_pos;
		current_pos = (current_pos+1)% max_packets;
		if(current_pos == 0){
			max_reached = true;
			//click_chatter("we reached max.. %llu",max_packets);
		}
		//you still have to allocate memory ..
		// do a new pkt()
	}
	//set indexOfMe = null
	//increase counts.
	return newpacket;
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(HashIndex)
ELEMENT_PROVIDES(PacketStore)
