// -*- related-file-name: "../../lib/signaturetable.cc" -*-
#ifndef SIGNATURE_TABLE_HH
#define SIGNATURE_TABLE_HH

#include <click/mcs_lock.h>
#include <string.h>
#include <stdlib.h>
#include <click/trieset.hh>

class ThreadLocalSignatureTable;

#define MAX_TOKENS_PER_SIG 8
struct token_st;
typedef struct sig_st{
  struct token_st* tokens;
  uint8_t count;
  uint8_t distances[MAX_TOKENS_PER_SIG];
  uint16_t idxes[MAX_TOKENS_PER_SIG];
  bool seen[MAX_TOKENS_PER_SIG];
} Signature;

typedef struct token_st{
  Signature** sigs;
  uint16_t count;
  char* label;
  uint32_t labellen;
} Token;


#define MAX_THREAD_COUNT 16

class GlobalSignatureTable{
  public:
    GlobalSignatureTable(){isMaster = false;}
    ~GlobalSignatureTable(){}
    void setIsMaster();
    void appendThreadLocalSignatureTable(ThreadLocalSignatureTable* t);
    static ThreadLocalSignatureTable** allTables;
    static uint32_t tablesCount;
    bool isMaster;
    static mcslock_t _lk; 
};

#define MAX_CONNECTIONS 1000

typedef struct connection_id_st{
  uint32_t ip1;
  uint32_t ip2;
  uint16_t sp1;
  uint16_t sp2;
}ConnectionID;

enum ConnStatus{
  ALLOW,
  DENY,
  PENDING,
  NO_RECORD
};

typedef struct conn_status_st{
  ConnectionID id;
  uint32_t initial_seqno_ip1;
  uint32_t initial_seqno_ip2;
  Signature* sigs;
  ConnStatus status;
}ConnRecord;


class ThreadLocalSignatureTable{
  friend class GlobalSignatureTable;

  public:
  ThreadLocalSignatureTable();
  ~ThreadLocalSignatureTable();

  bool addToPendingConnections(ConnectionID* id, uint32_t src_ip, uint32_t init_seqno);
  ConnStatus getStatus(ConnectionID* id);
  ConnRecord* getConnRecord(ConnectionID* id);
  ConnectionID pending_connections_queue[MAX_CONNECTIONS]; 
  volatile uint32_t pending_head;
  volatile uint32_t pending_tail;

  TrieSet* active_connections;
};

#endif
