/*
 * SynchIPRewriter.{cc,hh} -- rewrites packet source and destination
 * Max Poletto, Eddie Kohler
 *
 * Copyright (c) 2000 Massachusetts Institute of Technology
 * Copyright (c) 2008-2010 Meraki, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "synchiprewriter.hh"
#include <clicknet/ip.h>
#include <clicknet/tcp.h>
#include <clicknet/udp.h>
#include <click/args.hh>
#include <click/straccum.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/router.hh>
#include <ctime>
CLICK_DECLS

/**
 * NOTES: Only need to access lock for writes, not for reads.
 * HashContainer object used to back Map never does a background rehash,
 * rehash must *always* be called explicitly -- thus reads are SAFE
 * JUSTINE 24/10/2013
 **/

  SynchIPRewriter::SynchIPRewriter()
: _udp_map(0)
{
  _lk = 0;
  cur_threadid = 255;
  cur_thread_timesheld = 0;
  cur_virt_timestep = 0;

  //TODO: Add features to deal with loss and reordering of log packets.
  //
  replay_tids = new std::vector<uint8_t>();
  replay_tid_counts = new std::vector<uint16_t>();
  global_replay_timestep = 0;
  max_global_replay_timestep = 0;
  which_vector_we_are_at = 0;
  index_in_that_vector = 0;
  total_packets_processed = 0;
  gettimeofday(&firsttime, NULL);
}

SynchIPRewriter::~SynchIPRewriter()
{
}

  void *
SynchIPRewriter::cast(const char *n)
{
  if (strcmp(n, "IPRewriterBase") == 0)
    return (IPRewriterBase *)this;
  else if (strcmp(n, "TCPRewriter") == 0)
    return (TCPRewriter *)this;
  else if (strcmp(n, "SynchIPRewriter") == 0)
    return this;
  else
    return 0;
}

  int
SynchIPRewriter::configure(Vector<String> &conf, ErrorHandler *errh)
{
  bool has_udp_streaming_timeout = false;
  _udp_timeouts[0] = 60 * 5;	// 5 minutes
  _udp_timeouts[1] = 5;	// 5 seconds

  if (Args(this, errh).bind(conf)
      .read("UDP_TIMEOUT", SecondsArg(), _udp_timeouts[0])
      .read("UDP_STREAMING_TIMEOUT", SecondsArg(), _udp_streaming_timeout).read_status(has_udp_streaming_timeout)
      .read("UDP_GUARANTEE", SecondsArg(), _udp_timeouts[1])
      .consume() < 0)
    return -1;

  if (!has_udp_streaming_timeout)
    _udp_streaming_timeout = _udp_timeouts[0];
  _udp_timeouts[0] *= CLICK_HZ; // change timeouts to jiffies
  _udp_timeouts[1] *= CLICK_HZ;
  _udp_streaming_timeout *= CLICK_HZ; // IPRewriterBase handles the others

  return TCPRewriter::configure(conf, errh);
}

  inline IPRewriterEntry *
SynchIPRewriter::get_entry(int ip_p, const IPFlowID &flowid, int input)
{
  if (ip_p == IP_PROTO_TCP)
    return TCPRewriter::get_entry(ip_p, flowid, input);
  if (ip_p != IP_PROTO_UDP)
    return 0;
  IPRewriterEntry *m = _udp_map.get(flowid);
  if (!m && (unsigned) input < (unsigned) _input_specs.size()) {
    IPRewriterInput &is = _input_specs[input];
    IPFlowID rewritten_flowid = IPFlowID::uninitialized_t();
    if (is.rewrite_flowid(flowid, rewritten_flowid, 0, IPRewriterInput::mapid_iprewriter_udp) == rw_addmap)
      m = SynchIPRewriter::add_flow(0, flowid, rewritten_flowid, input);
  }
  return m;
}

  IPRewriterEntry *
SynchIPRewriter::add_flow(int ip_p, const IPFlowID &flowid,
    const IPFlowID &rewritten_flowid, int input)
{

  if (ip_p == IP_PROTO_TCP){
    IPRewriterEntry * ret = TCPRewriter::add_flow(ip_p, flowid, rewritten_flowid, input);
    return ret;
  }

  void *data;
  if (!(data = _udp_allocator.allocate()))
    return 0;

  IPRewriterInput *rwinput = &_input_specs[input];
  IPRewriterFlow *flow = new(data) IPRewriterFlow
    (rwinput, flowid, rewritten_flowid, ip_p,
     !!_udp_timeouts[1], click_jiffies() + relevant_timeout(_udp_timeouts));

  IPRewriterEntry * ret = store_flow(flow, input, _udp_map, &reply_udp_map(rwinput));
  return ret;
}

uint64_t test = 0;
  void
SynchIPRewriter::push(int port, Packet *p_in)
{
  if(port == 1){ //controlplane packet

#ifdef LOGTIMING
    //BLINKY POINTER FUN
    if(test == 0){
      gettimeofday(&firsttime, NULL);
    }else if (test == 24999){
      std::cout << "last cp pkt" << std::endl;
      gettimeofday(&lastcppkt, NULL);
    }
#endif

    const unsigned char * tail = p_in->end_data();
    uint64_t timestep = (uint64_t) *((uint64_t*) (tail - 8));
    uint16_t threadtimes = (uint16_t) *((uint16_t*) (tail - 10));
    uint8_t threadid = (uint8_t) *(tail - 11);

    assert(threadtimes != 0);
    test += 1;
    replay_tids->push_back(threadid);
    replay_tid_counts->push_back(threadtimes);
    max_global_replay_timestep += threadtimes;
    //uint8_t threadid = (uint8_t) p_in->data()[0];
    //uint8_t threadtimes = (uint8_t) p_in->data()[1];
    std::cout << "cp pkt:" << timestep << " " << (int) threadid << " " << (int) threadtimes  << " " << test<< std::endl;
    return;
  }
  //else it's a dataplane packet



  WritablePacket *p = p_in->uniqueify();
  click_ip *iph = p->ip_header();

  // handle non-first fragments
  if ((iph->ip_p != IP_PROTO_TCP && iph->ip_p != IP_PROTO_UDP)
      || !IP_FIRSTFRAG(iph)
      || p->transport_length() < 8) {
    const IPRewriterInput &is = _input_specs[port];
    if (is.kind == IPRewriterInput::i_nochange)
      output(is.foutput).push(p);
    else
      p->kill();
    return;
  }

  if(p->_pd_batch_id == 1 || p->_pd_batch_id == 3){
    acquire_map_lock(p);
  }
  else if(global_replay_timestep < max_global_replay_timestep) acquire_map_lock(p);


  //std::cout << "GRABBED LOCK FER " << (int) p->_parent_thread << " " << p->dst_ip_anno().unparse().data() << std::endl;
  cur_thread_timesheld++;
  IPFlowID flowid(p);
  HashContainer<IPRewriterEntry> *map = (iph->ip_p == IP_PROTO_TCP ? &_map : &_udp_map);
  IPRewriterEntry *m = map->get(flowid);

  if (!m) {			// create new mapping
    IPRewriterInput &is = _input_specs.at_u(port);
    IPFlowID rewritten_flowid = IPFlowID::uninitialized_t();
    int result = is.rewrite_flowid(flowid, rewritten_flowid, p, iph->ip_p == IP_PROTO_TCP ? 0 : IPRewriterInput::mapid_iprewriter_udp);
    if (result == rw_addmap)
      m = SynchIPRewriter::add_flow(iph->ip_p, flowid, rewritten_flowid, port);
    if (!m) {
      checked_output_push(result, p);
      return;
    } else if (_annos & 2)
      m->flow()->set_reply_anno(p->anno_u8(_annos >> 2));
  }

  click_jiffies_t now_j = click_jiffies();
  IPRewriterFlow *mf = m->flow();
  if (iph->ip_p == IP_PROTO_TCP) {
    TCPFlow *tcpmf = static_cast<TCPFlow *>(mf);
    tcpmf->apply(p, m->direction(), _annos);
    if (_timeouts[1])
      tcpmf->change_expiry(_heap, true, now_j + _timeouts[1]);
    else
      tcpmf->change_expiry(_heap, false, now_j + tcp_flow_timeout(tcpmf));
  } else {
    UDPFlow *udpmf = static_cast<UDPFlow *>(mf);
    udpmf->apply(p, m->direction(), _annos);
    if (_udp_timeouts[1])
      udpmf->change_expiry(_heap, true, now_j + _udp_timeouts[1]);
    else
      udpmf->change_expiry(_heap, false, now_j + udp_flow_timeout(udpmf));
  }

  output(m->output()).push(p);


#ifdef LOGTIMING
  total_packets_processed++;
  if(total_packets_processed == 1){
    long long diff = (lastcppkt.tv_sec - firsttime.tv_sec) * 1000000 + (lastcppkt.tv_usec - firsttime.tv_usec);
    std::cout << "STARTUP TIME WAS " << (diff) << std::endl;
  }
  if(total_packets_processed == 1 || total_packets_processed % 30000 == 0){
    struct timeval sysTime;
    gettimeofday(&sysTime, NULL);
    long long diff = (sysTime.tv_sec - firsttime.tv_sec) * 1000000 + (sysTime.tv_usec - firsttime.tv_usec); 
    std::cout << "TIME IS " << (diff) << " PKTS IS " << total_packets_processed << std::endl;
    gettimeofday(&firsttime, NULL);
  }
#endif

  if(p->_pd_batch_id == 2 || p->_pd_batch_id == 3) release_map_lock();
  else if(global_replay_timestep < max_global_replay_timestep) release_map_lock();
}

  String
SynchIPRewriter::udp_mappings_handler(Element *e, void *)
{
  SynchIPRewriter *rw = (SynchIPRewriter *)e;
  click_jiffies_t now = click_jiffies();
  StringAccum sa;
  //rw->acquire_map_lock(); TODO: FIX TIMERS
  for (Map::iterator iter = rw->_udp_map.begin(); iter.live(); ++iter) {
    iter->flow()->unparse(sa, iter->direction(), now);
    sa << '\n';
  }
  //rw->release_map_lock();
  return sa.take_string();
}

  void
SynchIPRewriter::add_handlers()
{
  add_read_handler("tcp_mappings", tcp_mappings_handler);
  add_read_handler("udp_mappings", udp_mappings_handler);
  add_rewriter_handlers(true);
}

  CLICK_ENDDECLS
  ELEMENT_REQUIRES(TCPRewriter UDPRewriter)
EXPORT_ELEMENT(SynchIPRewriter)
