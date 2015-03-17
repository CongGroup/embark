#ifndef CLICK_NBSynchIPRewriter_HH
#define CLICK_NBSynchIPRewriter_HH
////#define WRITELOCKLOG
#include "tcprewriter.hh"
#include "udprewriter.hh"
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <unistd.h>
#include <click/config.h>
#include <click/router.hh>
#include <click/master.hh>
CLICK_DECLS
class UDPRewriter;

class NBSynchIPRewriter : public TCPRewriter { public:

    typedef UDPRewriter::UDPFlow UDPFlow;

    NBSynchIPRewriter();
    ~NBSynchIPRewriter();

    virtual const char *port_count() const    { return "2/3"; }
    virtual const char *processing() const    { return "hh/hhh"; }


    const char *class_name() const		{ return "NBSynchIPRewriter"; }
    void *cast(const char *);

    int configure(Vector<String> &, ErrorHandler *);

    IPRewriterEntry *get_entry(int ip_p, const IPFlowID &flowid, int input);

    HashContainer<IPRewriterEntry> *get_map(int mapid) {
	    if (mapid == IPRewriterInput::mapid_default)
	      return &_map;
	    else if (mapid == IPRewriterInput::mapid_iprewriter_udp)
	      return &_udp_map;
	    else
	      return 0;
    }
    
    IPRewriterEntry *add_flow(int ip_p, const IPFlowID &flowid,
			      const IPFlowID &rewritten_flowid, int input);
    
    void destroy_flow(IPRewriterFlow *flow);
      click_jiffies_t best_effort_expiry(const IPRewriterFlow *flow) {
	    if (flow->ip_p() == IP_PROTO_TCP)
	      return TCPRewriter::best_effort_expiry(flow);
	    else
	      return flow->expiry() + udp_flow_timeout(static_cast<const UDPFlow *>(flow)) - _udp_timeouts[1];
    }

    void push(int, Packet *);

    void add_handlers();

  private:

    Map _udp_map;
    SizedHashAllocator<sizeof(UDPFlow)> _udp_allocator;
    uint32_t _udp_timeouts[2];
    uint32_t _udp_streaming_timeout;

    int udp_flow_timeout(const UDPFlow *mf) const {
  	  if (mf->streaming())
	        return _udp_streaming_timeout;
	     else
	        return _udp_timeouts[0];
    }

    static inline Map &reply_udp_map(IPRewriterInput *rwinput) {
	    NBSynchIPRewriter *x = static_cast<NBSynchIPRewriter *>(rwinput->reply_element);
	    return x->_udp_map;
    }
    static String udp_mappings_handler(Element *e, void *user_data);
 
    //FOR RECORD & REPLAY
    atomic_uint32_t _lk;

    struct timeval lastcppkt;
    struct timeval firsttime;
    uint64_t total_packets_processed; //For logging & measurement
    uint64_t cur_virt_timestep;
    uint64_t cur_threadid;
    uint16_t cur_thread_timesheld;

    std::vector<uint8_t>* replay_tids;
    std::vector<uint16_t>* replay_tid_counts;

    volatile uint32_t global_replay_timestep;
    volatile uint32_t max_global_replay_timestep;

    volatile uint32_t which_vector_we_are_at;
    volatile uint32_t index_in_that_vector;

    uint8_t get_replay_threadid(){
        uint64_t threadid = pthread_self(); 
 
        //std::cout << "checking gtid for " << threadid << std::endl;
        Master* m = router()->master();
        uint8_t global_tid = 255;
        for(uint8_t i = 0; i < m->nthreads(); i++){
          if(m->thread(i)->_running_processor == threadid){
            global_tid = i;
            break;
          }
        }
        assert(global_tid != 255);
     //   std::cout << "tid accessed " << (int) global_tid << std::endl;
        return global_tid;
    }


    void acquire_map_lock(){
      if(global_replay_timestep >= max_global_replay_timestep){
        while(_lk.compare_swap(0, 1) != 0);
        
#ifdef WRITELOCKLOG
          uint64_t threadid = pthread_self(); 
          cur_virt_timestep += 1;
          //char* data = new char[46];
          uint8_t global_tid = get_replay_threadid();    
          //data[35] = (char) global_tid;
          //memcpy(&(data[36]), &cur_thread_timesheld, 2);
          //memcpy(&(data[38]), &cur_virt_timestep, 8);
          //std::cout << "new control pkt:" << cur_virt_timestep << " " << (int) cur_thread_timesheld << " " << (int) global_tid << std::endl;
          //WritablePacket* newpkt = Packet::make_log_pkt(cur_virt_timestep, cur_thread_timesheld, global_tid);
          //WritablePacket* newpkt = Packet::make((const void*) data, 46);
          //output(2).push(newpkt);

          cur_threadid = threadid;
          cur_thread_timesheld = 0;
#endif
      }else{
        uint8_t global_tid = get_replay_threadid();
        uint8_t next_thread = replay_tids->at(which_vector_we_are_at);
        
        //std::cout << (int) global_tid << " trying to grab lock! " << std::endl;
        uint32_t my_next_turn = which_vector_we_are_at;
        while(replay_tids->size() > my_next_turn && replay_tids->at(my_next_turn) != global_tid) my_next_turn++;
        //std::cout << (int) global_tid << " next turn " << my_next_turn << std::endl;
        if(my_next_turn < replay_tids->size()){
          while(which_vector_we_are_at != my_next_turn);
        }
        //std::cout << (int) global_tid << " got out! " << std::endl;
        while(_lk.compare_swap(0, 1) != 0); //In case of last replay
        global_replay_timestep++;
   //     std::cout << "REPLAY LOCK ACQUIRED " << (int) global_tid << " " << global_replay_timestep << std::endl;
      }

      //std::cout << _lk << " Lock acquired by " << threadid << std::endl; 
    }

    void release_map_lock(){
    
      _lk.swap(0);
    }
};


inline void
NBSynchIPRewriter::destroy_flow(IPRewriterFlow *flow)
{
    acquire_map_lock();
    std::cout << "destroying flow" << std::endl;
    if (flow->ip_p() == IP_PROTO_TCP)
	TCPRewriter::destroy_flow(flow);
    else {
	unmap_flow(flow, _udp_map, &reply_udp_map(flow->owner()));
	flow->~IPRewriterFlow();
	_udp_allocator.deallocate(flow);
    }
    release_map_lock();
}

CLICK_ENDDECLS
#endif
