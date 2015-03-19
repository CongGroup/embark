#include <click/signaturetable.hh>
#include <click/mcs_lock.h>
#include <iostream>

mcslock_t GlobalSignatureTable::_lk;
uint32_t GlobalSignatureTable::tablesCount = 0;
ThreadLocalSignatureTable** GlobalSignatureTable::allTables;

//Note: Master should be created at config time; children should be created at init time.
//Master resides at GarbleSpeaker.
//Threads are BlindBoxFilters 
void GlobalSignatureTable::setIsMaster(){
  std::cout << "Setting up Master global signature table!" << std::endl;
  GlobalSignatureTable::allTables = (ThreadLocalSignatureTable**) malloc(sizeof(ThreadLocalSignatureTable*) * MAX_THREAD_COUNT);
  GlobalSignatureTable::tablesCount = 0;
  mcs_lock_init(&(GlobalSignatureTable::_lk));
  isMaster = true;
}

//Called at init time by threads.
void GlobalSignatureTable::appendThreadLocalSignatureTable(ThreadLocalSignatureTable* t){
  _mcslock_node lock_node;
  mcs_lock(&(GlobalSignatureTable::_lk), &lock_node);
  GlobalSignatureTable::allTables[tablesCount] = t;
  GlobalSignatureTable::tablesCount++;
  mcs_unlock(&(GlobalSignatureTable::_lk), &lock_node);
}

/*****
//ThreadLocal stuff
*******/

ThreadLocalSignatureTable::ThreadLocalSignatureTable(){
  memset(pending_connections_queue, 0, MAX_CONNECTIONS * sizeof(uint32_t));
  pending_head = 0;
  pending_tail = 0;

  active_connections = (TrieSet*) malloc(sizeof(TrieSet));
}

bool ThreadLocalSignatureTable::addToPendingConnections(ConnectionID* id, uint32_t src_ip, uint32_t init_seqno){
        if(pending_tail - pending_head >= MAX_CONNECTIONS) return false;
    
        memcpy(&(pending_connections_queue[pending_tail]), id, sizeof(ConnectionID));
        void* record = malloc(sizeof(ConnRecord));
        memset(record, 0, sizeof(ConnRecord));
        ((ConnRecord*)record)->status = PENDING;
        
        //Set the initial sequence number -- we need this so we can
        //count how far in to the connection we are. We need to allow
        //the SSL handshake through, but nothing more.
        if(src_ip == id->ip1){
          ((ConnRecord*)record)->initial_seqno_ip1 = init_seqno;
        }else if(src_ip == id->ip2){
          ((ConnRecord*)record)->initial_seqno_ip2 = init_seqno;
        }else{
          std::cout << "WHAT THE HECK? (ThreadLocalSignatureTable)" << std::endl;
          exit(5);
        }


        active_connections->if_not_contains_then_insert((unsigned char*) id, sizeof(ConnectionID), record);
        pending_tail++;
        return true;
}

ConnStatus ThreadLocalSignatureTable::getStatus(ConnectionID* id){
  void* data = active_connections->get_related_data((unsigned char*) id, sizeof(id));
  if(data == 0){
    return NO_RECORD;
  }else{
    return ((ConnRecord*)data)->status;
  }
}

ConnRecord* ThreadLocalSignatureTable::getConnRecord(ConnectionID* id){
  return (ConnRecord*) active_connections->get_related_data((unsigned char*) id, sizeof(id));
}
