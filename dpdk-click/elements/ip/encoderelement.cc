#include <click/config.h>
#include "encoderelement.hh"
#include <clicknet/ip.h>
#include <click/confparse.hh>
#include "index.hh"
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

/*
   redundancy elemnt code file.
 */


EncoderElement::EncoderElement(){
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
EncoderElement::~EncoderElement(){
  delete index;
  delete store;
}

int EncoderElement::configure(Vector<String> &conf, ErrorHandler *errh){
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
  setupTable();
  max_packets = store_size / sizeof(pkt);
  current_uuid = max_packets + 1;
  index = new hashTable(max_packets);
  //click_chatter("setup worked!");
  store = new packetStore(store_size);
  click_chatter("setup worked!");
  /*We'll be soon setting up the hash Index and packet store*/

  gettimeofday(&timeVal2, 0);
  micro2 = (double)timeVal2.tv_sec * 1000000 + timeVal2.tv_usec;

  double micro = ( micro2 - micro1);
  double sec = micro/(1000000);

  click_chatter(" setup time: %lf %lf \n",sec,micro);


  return 0;
}


void EncoderElement::push(int, Packet *p){
  packetcount++;
  if(packetcount%10000==0){
    //click_chatter("%llu %llu %llu %f\n",packetcount,redundant,totalbits,
    //	  ((float)redundant/(float)totalbits));
  }
  Packet *a = computeHash(p);
#if FINEST
  click_chatter(" computed and encoded packet \n");
#endif
  if(a == NULL){
    output(0).push(p);
  }
  else{
    output(0).push(a);
    p->kill();
  }

}

inline unsigned char EncoderElement::sigmask_isAGoodHash(uint64_t hash) {
  return((hash & sigmask) == 0);
}

unsigned short EncoderElement::walk_left(
    const unsigned char *pStore,
    const unsigned char *pHere,
    unsigned short redundancy_limit)
{
  unsigned short duplicate_bytes = 0;
  while((*pHere == *pStore)&&(duplicate_bytes < redundancy_limit)){
    pHere--;
    pStore--;
    duplicate_bytes++;
  }
  return duplicate_bytes;
}
unsigned short EncoderElement::walk_right(
    const unsigned char *pStore,
    const unsigned char *pHere,
    unsigned short redundancy_limit)
{
  unsigned short duplicate_bytes = 0;
  while((*pHere == *pStore)&&(duplicate_bytes < redundancy_limit)){
    pStore++;
    duplicate_bytes++;
    pHere++;
  }
  return duplicate_bytes;
}


void inline EncoderElement::hash_advanceHash(uint64_t *pHash,
    const unsigned char *pRightLimit, 
    const unsigned char **ppLeftEdgeByte,
    const unsigned char **ppRightEdgeByte)
{
  uint64_t hash=*pHash;
  const unsigned char *pLeftEdgeByte  = *ppLeftEdgeByte;
  const unsigned char *pRightEdgeByte = *ppRightEdgeByte;
  if(pRightEdgeByte < pRightLimit) do {
    hash = (hash * (uint64_t) prime + (uint64_t)(*pRightEdgeByte) +1)
      & hashmask;
    hash = (hash + (uint64_t) table[(int)(*pLeftEdgeByte)])
      & hashmask;
    pLeftEdgeByte++;
    pRightEdgeByte++;
  } while(!sigmask_isAGoodHash(hash) && pRightEdgeByte < pRightLimit);
  *ppLeftEdgeByte  = pLeftEdgeByte;
  *ppRightEdgeByte = pRightEdgeByte;
  *pHash=hash;
}

int inline EncoderElement::hash_testAndInsert(uint64_t hash, pkt *pPkt, unsigned short offset, unsigned short *pRedundantLeftBound,int inserts)
{
  hashEntry *pHashEntry = index->lookup(hash, max_packets, current_uuid);
  //
  //check validity of packet using store
  int flag = 0;
  if(pHashEntry){
    if(*pRedundantLeftBound <= offset){
      // we wont encode  with respeect to current packet..
      if(pHashEntry->_uuid == current_uuid) return 0;
      collision_detect(pHashEntry,
          pPkt,
          offset,
          pRedundantLeftBound, inserts);
      represent[inserts].fprint = hash;
      represent[inserts].offset = offset;

      //update the /atchinfo
      matchinfo[inserts].fprint = hash;
      matchinfo[inserts].uuid = pHashEntry->_uuid;
      matchinfo[inserts].offset = pHashEntry->_offset;			
      flag = 1;
    }
  }
  else{
    (void)index->insert(hash,offset,current_uuid, max_packets, current_uuid);
    flag = 0;
  }
  return flag;
}

void EncoderElement::collision_detect(hashEntry *pHashEntry,
    pkt *pPkt, unsigned short offset, 
    unsigned short *pleftbound, int inserts)
{
  unsigned short n;
  unsigned short l;

  if(pHashEntry->_uuid < (current_uuid - max_packets)){
    click_chatter("error\n");
  }
  pkt *old_packet = store->getPacket(pHashEntry->_uuid, current_uuid);
  if(old_packet == NULL){
    click_chatter("This is clearly fatal adn should seg fault %llu %llu\n", pHashEntry->_uuid,current_uuid);
  }

  n = walk_right(old_packet->packetData()+pHashEntry->_offset,
      pPkt->packetData()+offset,
      MINIMA(pPkt->_length-offset,
        old_packet->_length-pHashEntry->_offset));
  l = walk_left(old_packet->packetData()+pHashEntry->_offset-1,
      pPkt->packetData()+offset-1,
      MINIMA(pHashEntry->_offset,
        offset-*pleftbound));
  *pleftbound = offset+n;
  unsigned short direct = l+n;
  represent[inserts].left = (uint16_t)l;
  represent[inserts].right = (uint16_t)n;
  redundant += direct;
}

inline void EncoderElement::hash_testAndNoCollide(uint64_t hash,  unsigned short offset)
{
  hashEntry *pHashEntry = index->lookup(hash, max_packets, current_uuid);
  //check validity of packt using store
  if(pHashEntry){
    (void)index->replace(pHashEntry,offset,current_uuid);
  }
  else{
    (void)index->insert(hash,offset,current_uuid, max_packets, current_uuid);
  }
}


Packet *EncoderElement::computeHash(Packet *p){
  /* 
     The check for IP Header is assumed to have been made.
     If not TCP or UDP we don't do anything!
   */


  const click_ip *ipheader = p->ip_header();
  uint64_t fprint = 0;
  uint64_t hash = 0;
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
    delete source;
    delete dest;
    return NULL;
  }
  const unsigned char *data = p->data();
  data += iphlen+thlen;

  uint32_t payloadlength = p->length() - iphlen - thlen;
  if(p->length() < (iphlen+thlen)){
    //click_chatter("something and weirdly fatally wrong?\n");
  }

  if(payloadlength < siglen){ 
    totalbits += payloadlength;
    WritablePacket *q = Packet::make(iphlen+thlen+payloadlength + 1); 

    memcpy(q->data()+iphlen, p->data()+iphlen,thlen);
    *(q->data() + iphlen + thlen ) = 0;
    memcpy(q->data() + iphlen + thlen + 1, p->data() + iphlen + thlen ,payloadlength);


    // checksum computation is done at the end.

    click_ip iph;
    memcpy(&iph,p->ip_header(),sizeof(click_ip));
    iph.ip_len = htons(iphlen + thlen + payloadlength + 1);
    iph.ip_sum = 0;
    iph.ip_sum = click_in_cksum((unsigned char *)&iph,iphlen);
    memcpy(q->data(),&iph,iphlen);
    delete source;
    delete dest;
    return q;
  }


  else if(payloadlength > 1464){
    //click_chatter("something fatally wrong?\n");
    delete source;
    delete dest;
    return NULL;
  }

  else{

    current_uuid++;
    totalbits += payloadlength;
    uint32_t limit = MINIMA(siglen,payloadlength);
    //add packet to packetstore right about now.
    pkt *newpacket = store->addPacket(data,source, dest, sport, dport, proto, payloadlength);
    unsigned short redundant_left_bound = 0;
    unsigned short inserts;
    int retflag = 0;
    int32_t redunpktlength = payloadlength;
    const unsigned char *pRightWindowEdgeByte = &data[limit];
    const unsigned char *pLeftWindowEdgeByte = &data[0];
    const unsigned char *pEndOfPacket = &data[payloadlength];
    const unsigned char *begin = data;
    const unsigned char *end = data + limit;
    for(;data < end; data++){
      fprint = (fprint*prime + (uint64_t)((int)1+(int)(*data))) & hashmask; 
    }
    hash = fprint;
    inserts = 0;
    int num_fingerprints = 0;
#define MAXFP 1000
    uint64_t fingerprints[MAXFP];
    fingerprints[num_fingerprints] = fprint;

    do{
      if(inserts < MAXINSERTS){
        retflag = hash_testAndInsert(hash,newpacket,pLeftWindowEdgeByte-begin,&redundant_left_bound, inserts);
        if(retflag == 1){
          redunpktlength += (sizeof(Encode) - represent[inserts].left - represent[inserts].right);
          inserts++;
        }
      }
      else{
        hash_testAndNoCollide(hash,pLeftWindowEdgeByte-begin);
      }
      num_fingerprints ++;
      fingerprints[num_fingerprints] = hash;
      hash_advanceHash(&hash, pEndOfPacket, &pLeftWindowEdgeByte, &pRightWindowEdgeByte);

    }while(pRightWindowEdgeByte < pEndOfPacket && num_fingerprints < MAXFP);
    if(inserts == 0){
      // let's add one byte to just store that packet is encoded or not.
      // in future, we will use a bit in packet header to do so - really need a bit information

      WritablePacket *q = Packet::make(iphlen+thlen+payloadlength + 1); // adding one byte for encoding information, can use annotator??
      click_ip iph;
      memcpy(&iph,p->ip_header(),sizeof(click_ip));
      iph.ip_len = htons(iphlen + thlen + payloadlength + 1);
      iph.ip_sum = 0;	
      iph.ip_sum = click_in_cksum((unsigned char *)&iph,iphlen);

      memcpy(q->data(),&iph,iphlen);
      memcpy(q->data()+iphlen, p->data()+iphlen,thlen);

#if FINER
      click_chatter(" setting first byte of packet to be non-encoded \n");
#endif
      *(q->data() + iphlen + thlen ) = 0;
      memcpy(q->data() + iphlen + thlen + 1, p->data() + iphlen + thlen ,payloadlength);	
      return q;
    }
    else{
      WritablePacket *q = Packet::make(iphlen+thlen+redunpktlength + 1); // adding one byte for encoding information, can use annotator?? 
      if(q == NULL){
        click_chatter("This is fatal - mem. allocation\n");
      }
      else{
#if FINER
        click_chatter(" inserts is > 0, so encoding \n");
#endif
        click_ip iph;
        memcpy(&iph,p->ip_header(),sizeof(click_ip));
        iph.ip_len = htons(iphlen + thlen + redunpktlength + 1);
        iph.ip_sum = 0;
        iph.ip_sum = click_in_cksum((unsigned char *)&iph,iphlen);
        memcpy(q->data(),&iph,iphlen);
        memcpy(q->data()+iphlen, p->data()+iphlen,thlen);

        uint16_t beginp = iphlen+thlen;
        *(q->data() + beginp) = (unsigned char)(inserts ) + 0x80;
        beginp ++;

        uint16_t begin_e = 0;
        int16_t end_e = 0;
        Encode *epObj;
        for(int i = 0; i < inserts; i++){
          epObj = (Encode *) (q->data() + beginp);
          epObj->uuid = htonq(matchinfo[i].uuid);
          epObj->offset = htons(matchinfo[i].offset - represent[i].left);
          epObj->length = htons(represent[i].left + represent[i].right);
          epObj->my_offset = htons(represent[i].offset - represent[i].left);
#if FINER
          click_chatter(" Encoding info: match uuid %lu offset %d length %d my_offset %d current uuid %lu\n",
              ntohq(epObj->uuid),ntohs(epObj->offset),ntohs(epObj->length),ntohs(epObj->my_offset),current_uuid);
#endif

          beginp += sizeof(Encode);
        }

        for(int i = 0; i < inserts; i++){
          end_e = represent[i].offset - represent[i].left;
          assert(end_e >= 0);
#if FINER
          click_chatter(" inserts: %d copying data: %d to %d to encoded\n",i,begin_e,end_e); 
#endif				
          memcpy(q->data()+beginp,p->data()+iphlen+thlen+begin_e,end_e-begin_e);
          beginp += (end_e - begin_e);
          begin_e = represent[i].offset+represent[i].right;
          // for the last insert, insert the remaining content as well
          if(i == inserts - 1){
#if FINER
            click_chatter(" copying remaining %d to %d\n",begin_e, payloadlength);
#endif
            memcpy(q->data() + beginp, p->data() + iphlen + thlen + begin_e,payloadlength - begin_e);
          }


        }


      }


      return q;
    }
  }
}

void EncoderElement::setupTable(){
  uint64_t pn = 1;
  unsigned int i,j;
  for(j=0; j<siglen; j++){
    pn = pn*prime & hashmask;
  }
  for(i=0; i<256; i++){
    table[i] = -( ((i+1)*pn) & hashmask);
  }
}
uint64_t EncoderElement::hash_getmaskoflength(unsigned int new_masklen) {
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
ELEMENT_REQUIRES(PacketStore HashIndex)
EXPORT_ELEMENT(EncoderElement)
ELEMENT_MT_SAFE(EncoderElement)
