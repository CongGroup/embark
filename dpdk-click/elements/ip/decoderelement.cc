#include <click/config.h>
#include "decoderelement.hh"
#include <clicknet/ip.h>
#include <click/confparse.hh>
#include "packetstore.hh"
#include <click/error.hh>
#include <click/glue.hh>
#include <clicknet/tcp.h>
#include <click/integers.hh>
#include <clicknet/udp.h>

#include <sys/time.h>

#define DBG 1
#define DBG_FINE 1
#define DBG_FINER 0
#define DBG_FINEST 0
#define FINEST (DBG & DBG_FINEST)
#define FINER (DBG & DBG_FINER)
#define FINE (DBG & DBG_FINE)

CLICK_DECLS

  DecoderElement::DecoderElement(){
  	prime = 1048583;
  	hashmask = 0x3fffffffffffffffULL; /* 60 bits */
  	siglen = 64;
	sigmask = 31ULL;
	totalbits = 0;
	redundant = 0;
	packetcount = 0;
	current_uuid = 0;
	click_chatter("set up done here-AA");
}
  DecoderElement::~DecoderElement(){
	  delete receiver_store;
  }
  
  int DecoderElement::configure(Vector<String> &conf, ErrorHandler *errh){
  	struct timeval timeVal1,timeVal2;
	double micro1 = -1;
	double micro2 = -1;

	gettimeofday(&timeVal1, 0);
	micro1 = (double)timeVal1.tv_sec * 1000000 + timeVal1.tv_usec;

	uint32_t hmask_length = 60;
	uint32_t smask_length = 7;
	uint64_t store_size = 419430400; 
	if(cp_va_parse(conf,this,errh,cpOptional,cpKeywords,"PRIME",cpUnsigned64,"Prime Number",&prime,
		"WINDOW_LENGTH",cpUnsignedShort,"Window over which hash is computed",&siglen,
		"HASHMASK",cpUnsigned,"Length in bits of HashMask",&hmask_length,
		"SIGMASK",cpUnsigned,"Length in bits of Fingerprint Mask",&smask_length,
		"STORESIZE",cpUnsigned64,"Size of store in bits",&store_size,
		cpEnd) < 0){
		return -1;
	}
	/*Setup hashmask and sigmask*/
	hashmask = hash_getmaskoflength(hmask_length);
	sigmask = hash_getmaskoflength(smask_length);
	
	/*Setup table here*/
	max_packets = store_size / sizeof(pkt);
	receiver_current_uuid = max_packets + 1;
	receiver_store = new packetStore(store_size);
	click_chatter("setup worked!");

       	gettimeofday(&timeVal2, 0);
	micro2 = (double)timeVal2.tv_sec * 1000000 + timeVal2.tv_usec;
	double micro = ( micro2 - micro1);
	double sec = micro/(1000000);
	click_chatter(" setup time: %lf %lf \n",sec,micro);
	return 0;
}
  

  void DecoderElement::push(int, Packet *p){
	  packetcount++;
	  Packet *a = decodePacket(p);
	  if(a == NULL){
		  output(0).push(p);
	  }
	  else{
		  output(0).push(a);
		  p->kill();
	  }
  }
 
Packet *DecoderElement::decodePacket(Packet *p){
	/* 
	   The check for IP Header is assumed to have been made.
	   If not TCP or UDP we don't do anything!
	*/
	const click_ip *ipheader = p->ip_header();
	uint32_t iphlen = ipheader->ip_hl << 2;
	uint32_t thlen = 0;
	IPAddress *source = new IPAddress(ipheader->ip_src);
	IPAddress *dest = new IPAddress(ipheader->ip_dst);
	uint16_t sport, dport;
	uint8_t proto = ipheader->ip_p;
	if(ipheader->ip_p == IP_PROTO_TCP){	
		const click_tcp *tcpheader = p->tcp_header();
		thlen = tcpheader->th_off << 2;
		sport = tcpheader->th_sport;
		dport = tcpheader->th_dport;
	}
	else if(ipheader->ip_p == IP_PROTO_UDP){
		const click_udp *udpheader = p->udp_header();
		thlen = (uint32_t)(sizeof(struct click_udp));
		sport = udpheader->uh_sport;
		dport = udpheader->uh_dport;
	}
	else{
		return NULL;
	}
	const unsigned char *data = p->data();
	data += iphlen+thlen;
	
	uint32_t payloadlength = p->length() - iphlen - thlen;
#if FINER
	click_chatter(" payloadlength %d \n",payloadlength);
#endif
	
	if(p->length() < (iphlen+thlen)){
		//click_chatter("something and weirdly fatally wrong?\n");
	}
	

	if(payloadlength > 1465){
		return NULL;
	}
	else{
#if FINER
		click_chatter(" decoding packet\n");
#endif
	// check the first byte to know if packet is encoded or not.
	uint16_t begin_pkt = 0;
	if(*(p->data()+iphlen + thlen) == 0x0){
		// packet is not encoded
		// simply store it.
		// and send the actual packet.
#if FINER
		click_chatter(" packet is not encoded \n");
#endif
				
		// create decoded packet
		WritablePacket *q = NULL;
		q = Packet::make(iphlen+thlen+payloadlength-1);  
		click_ip iph;
		memcpy(&iph,p->ip_header(),sizeof(click_ip));
		iph.ip_len = htons(iphlen + thlen + payloadlength - 1);
		iph.ip_sum = 0;
		iph.ip_sum = click_in_cksum((unsigned char *)&iph,iphlen);

		memcpy(q->data(),&iph,iphlen);
		memcpy(q->data()+iphlen, p->data()+iphlen,thlen);
		memcpy(q->data()+iphlen + thlen, p->data() + iphlen + thlen+1,payloadlength - 1); 

		// if payloadlength - 1 is < siglen, do not store the packet
		totalbits += payloadlength - 1;
		if(payloadlength - 1 < siglen)
			return q;
		
		receiver_store->addPacket(p->data() + iphlen + thlen + 1, source, dest, sport, dport, proto, payloadlength-1);
		receiver_current_uuid ++;
		return q;

	} else {
		// for encoded packet.
#if FINER
		click_chatter(" packet is encoded \n");
#endif
		int16_t inserts = int16_t(*(p->data() + iphlen + thlen + begin_pkt) & 0x7f);
#if FINER
		click_chatter(" number of inserts : %d \n",inserts);
#endif

		uint16_t begin = 1;
		uint16_t begin_dc = 0;
		uint16_t decoded_datalength = 0;
		int16_t end = 0;

		for(int i = 0; i < inserts; i ++){
			
			encodeobjs[i].uuid = ntohq(*((uint64_t *)(p->data() + iphlen + thlen + begin)));
			encodeobjs[i].offset = ntohs(*((uint16_t *)( p->data() + iphlen + thlen + begin + 8)));
			encodeobjs[i].length = ntohs(*((uint16_t *)(p->data() + iphlen + thlen + begin + 10)));
			encodeobjs[i].my_offset = ntohs(*((uint16_t*)(p->data() + iphlen + thlen + begin + 12)));
			redundant += encodeobjs[i].length;
			begin += sizeof(Encode);
		}

		for(int i=0; i < inserts; i ++){
#if FINER
			click_chatter(" insert info: uuid %lu offset %d length %d my_offset %d current_uuid %lu \n",encodeobjs[i].uuid, encodeobjs[i].offset,
									encodeobjs[i].length, encodeobjs[i].my_offset,receiver_current_uuid);
#endif
			end = encodeobjs[i].my_offset;
			// first copy the data up to first offset to decoded_data
			memcpy(decoded_data + begin_dc,p->data() + iphlen + thlen + begin, end -begin_dc);
			begin += end - begin_dc;
			decoded_datalength += end - begin_dc;
			begin_dc = end;
			// then find the matched packet
			begin_dc += encodeobjs[i].length;
			decoded_datalength += encodeobjs[i].length;
			// if it is last insert, copy the remaining.
			if(i == inserts - 1){
				memcpy(decoded_data + begin_dc, p->data() + iphlen + thlen + begin,payloadlength - begin );
				decoded_datalength += payloadlength - begin;
			}
		}

		for(int i = 0; i < inserts ; i ++){
			if(encodeobjs[i].uuid == receiver_current_uuid + 1){
				memcpy(decoded_data + encodeobjs[i].my_offset,decoded_data + encodeobjs[i].offset, encodeobjs[i].length);
			}
			else{
				pkt *match_pkt;
				match_pkt= receiver_store->getPacket(encodeobjs[i].uuid,receiver_current_uuid);
				memcpy(decoded_data + encodeobjs[i].my_offset,match_pkt->packetData() + encodeobjs[i].offset, encodeobjs[i].length);
			}

		}

		totalbits += decoded_datalength;
		receiver_store->addPacket((const unsigned char *)decoded_data,source,dest,sport,dport,proto,decoded_datalength);
		receiver_current_uuid ++;
	
		// create decoded packet.
		WritablePacket *q = Packet::make(iphlen+thlen+decoded_datalength);  
		click_ip iph;
		memcpy(&iph,p->ip_header(),sizeof(click_ip));
		iph.ip_len = htons(iphlen + thlen + decoded_datalength);
		iph.ip_sum = 0;
		iph.ip_sum = click_in_cksum((unsigned char *)&iph,iphlen);
		memcpy(q->data(),&iph,iphlen);
		memcpy(q->data()+iphlen, p->data()+iphlen,thlen);
		memcpy(q->data()+iphlen + thlen, decoded_data,decoded_datalength); 
		return q; 
	}
	return NULL;
	}
}

uint64_t DecoderElement::hash_getmaskoflength(unsigned int new_masklen) {
  uint64_t mask;
  if(new_masklen < 64) {
   mask = 0x7fffffffffffffffULL;
    mask   >>= (63 - new_masklen);
  } else {
    mask = (uint64_t)0xffffffffffffffffULL;
  }
  return mask;
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(PacketStore)
EXPORT_ELEMENT(DecoderElement)
ELEMENT_MT_SAFE(DecoderElement)
